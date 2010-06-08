#include "lib/universal_include.h"
#include "ftp_manager.h"
#include "lib/string_utils.h"
#include "lib/tosser/directory.h"
#include "lib/debug_utils.h"
#include "lib/netlib/net_mutex.h"
#include "network/network_defines.h"
#include <string.h>

#define FTP_MANAGER_VERBOSE_DEBUG
#define FTP_BLOCK_SIZE 1024
#define NUM_ACKS_TO_SEND 3

FTP::FTP( const char *_filename, const char *_data, int _size )
:	m_filename( newStr( _filename ) ),
	m_data( new char[_size] ),
	m_size( _size ),
	m_numBlocks( (_size + FTP_BLOCK_SIZE - 1) / FTP_BLOCK_SIZE )
{
	m_blockInfo = new BlockInfo[ m_numBlocks ];
	m_numBlocksRemaining = m_numBlocks;

	if( _data )
		memcpy( m_data, _data, _size );
}


FTP::~FTP()
{
	delete[] m_filename;
	delete[] m_data;
	delete[] m_blockInfo;
}


const char *FTP::Name() const
{
	return m_filename;
}


char *FTP::Data()
{
	return m_data;
}


int FTP::Size() const
{
	return m_size;
}


int FTP::NumBlocks() const
{
	return m_numBlocks;
}


int FTP::BlockSize( int _blockNumber ) const
{
	int offset = _blockNumber * FTP_BLOCK_SIZE;
	int bytesLeft = m_size - offset;
	if (bytesLeft > FTP_BLOCK_SIZE)
		return FTP_BLOCK_SIZE;
	else if (bytesLeft < 0)
		return 0;
	else
		return bytesLeft;
}


char *FTP::BlockData( int _blockNumber )
{
	int offset = _blockNumber * FTP_BLOCK_SIZE;

	if (offset > m_size)
		return NULL;
	else
		return m_data + offset;
}


bool FTP::BlockDone( int _blockNumber ) const
{
	return m_blockInfo[_blockNumber].m_done;
}


Directory *FTP::MakeBlockSendLetter( int _blockNumber )
{
	Directory *letter = new Directory;
	double now = GetHighResTime();
	
	letter->SetName( NET_DARWINIA_MESSAGE );
	letter->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_FILESEND );
	letter->CreateData( NET_DARWINIA_SEQID, -1 );
	letter->CreateData( NET_DARWINIA_FILENAME, Name() );
	letter->CreateData( NET_DARWINIA_FILESIZE, Size() );
	letter->CreateData( NET_DARWINIA_BLOCKNUMBER, _blockNumber );
	letter->CreateData( NET_DARWINIA_BLOCKDATA, BlockData( _blockNumber ), BlockSize( _blockNumber ) );

	m_blockInfo[_blockNumber].m_sendTime = now;
	m_blockInfo[_blockNumber].m_timedOut = false;

	return letter;
}


bool FTP::CanSendBlock( int _blockNumber ) const
{	
	return 
		!m_blockInfo[_blockNumber].m_done &&
		m_blockInfo[_blockNumber].m_timedOut;
}


int FTP::TimeOutBlocks( double _roundTripTime )
{
	double now = GetHighResTime();
	int numTimedOut = 0;

	for( int i = 0; i < m_numBlocks; i++ )
	{
		BlockInfo &b = m_blockInfo[i];

		if( !b.m_timedOut )
		{
			if( now > b.m_sendTime + _roundTripTime * 3 )
			{
				b.m_timedOut = true;
				numTimedOut++;

#ifdef 	FTP_MANAGER_VERBOSE_DEBUG
				if( numTimedOut )
				{
					AppDebugOut( "FTP %s timed out block %d\n", Name(), i );					
				}
#endif
			}
		}
	}

	return numTimedOut;
}


bool FTP::MarkBlockDone( int _blockNumber )
{
	// Returns true if the block hadn't timedout by the time it was acknowledged;

	bool result = false;

	BlockInfo &b = m_blockInfo[_blockNumber];

	if( !b.m_done )
	{		
		m_numBlocksRemaining--;
		result = !b.m_timedOut;
	}

	b.m_done = true;
	return result;
}


double FTP::GetBlockSendTime( int _blockNumber ) const
{
	return m_blockInfo[_blockNumber].m_sendTime;
}


bool FTP::Complete() const
{
	return m_numBlocksRemaining == 0;
}

//
// AckSet
//////////////////////

AckSet::AckSet( const char *_filename )
:	m_filename( newStr( _filename ) ),
	m_blockInfo( NULL ),
	m_numBlocks( 0 ),
	m_lastActive( GetHighResTime() )
{
}


AckSet::~AckSet()
{
	delete[] m_blockInfo;
	delete[] m_filename;
}


void AckSet::MarkAckToSend( int _blockNumber )
{
	if( _blockNumber >= m_numBlocks )
	{
		int numBlocks = _blockNumber + 100;
		BlockInfo *blockInfo = new BlockInfo[numBlocks];		

		if( m_blockInfo )
		{
			for( int i = 0; i < m_numBlocks; i++ )
				blockInfo[i] = m_blockInfo[i];

			delete[] m_blockInfo;
		}

		m_numBlocks = numBlocks;
		m_blockInfo = blockInfo;
	}

	double now = GetHighResTime();

	m_blockInfo[_blockNumber].m_acksRemaining = NUM_ACKS_TO_SEND;
	m_blockInfo[_blockNumber].m_timeReceived = now;
	m_lastActive = now;
}


int AckSet::AckBlock( int _blockNumber, double &_receivedTime ) 
{
	BlockInfo &b = m_blockInfo[_blockNumber];

	if( b.m_acksRemaining > 0)
	{
		_receivedTime = b.m_timeReceived;
		return b.m_acksRemaining--;
	}

	return false;
}


const char *AckSet::Name() const
{
	return m_filename;
}


int AckSet::NumBlocks() const
{
	return m_numBlocks;
}


double AckSet::LastActive() const
{
	return m_lastActive;
}


//
// FTPManager
//////////////////////

FTPManager::FTPManager( const char *_name )
:	m_name( newStr(_name) ),
	m_windowUsed( 0 ),
	m_windowSize( 10 ),
	m_roundTripTime( 0.1 ),
	m_lastAckTime( 0.0 ),
	m_mutex( new NetMutex() )
{
}


FTPManager::~FTPManager()
{
	Reset();
	delete m_mutex;
	delete[] m_name;
}


void FTPManager::SendFile( const char *_filename, const char *_data, int _size )
{
	if (m_sending.GetData( _filename ) != NULL) 
	{
		AppDebugOut("FTPManager[%s]: Already sending file %s\n", m_name, _filename );
	}
	else 
	{
		FTP *ftp = new FTP( _filename, _data, _size );
		m_sending.PutData( _filename, ftp );
		AppDebugOut("FTPManager[%s]: Starting to send file %s (%d bytes, %d blocks)\n", m_name, _filename, _size, ftp->NumBlocks() );
	}
}


int FTPManager::MakeSendLetters( LList<Directory *> &_lettersToSend ) 
{
	int numPackets = 0;

	int size = m_sending.Size();
	for (int i = 0; i < size; i++)
	{
		if( m_sending.ValidIndex( i ) )
		{
			FTP *ftp = m_sending[ i ];

			int numTimedOut = ftp->TimeOutBlocks( m_roundTripTime );
			m_windowUsed -= numTimedOut;
			m_windowSize -= numTimedOut;

			if( m_windowSize < 1 )
				m_windowSize = 1;

			numPackets += MakeSendLettersData( ftp, _lettersToSend );
		}
	}

	size = m_ackSets.Size();
	for (int i = 0; i < size; i++)
	{
		if( m_ackSets.ValidIndex( i ) )
		{
			AckSet *ackSet = m_ackSets[ i ];
			
			int numAcksToSend = MakeSendLettersAck( ackSet, _lettersToSend );
			numPackets += numAcksToSend;

			if( numAcksToSend == 0 )
			{
				// No acks were sent, and the ackSet has not been active for at least 5 seconds
				// let's destroy it
				m_ackSets.MarkNotUsed( i );
				delete ackSet;
			}			
		}
	}

#ifdef FTP_MANAGER_VERBOSE_DEBUG
	if (numPackets > 0)	
		AppDebugOut("FTPManager[%s]: Sent %d packets\n", m_name, numPackets);
#endif

	return numPackets;
}


int FTPManager::MakeSendLettersData( FTP *_ftp, LList<Directory *> &_lettersToSend )
{
	int numBlocks = _ftp->NumBlocks();
	int numPacketsSent = 0;
	double now = GetHighResTime();

	for (int blockNumber = 0; blockNumber < numBlocks; blockNumber++)
	{			
		// Don't send more than m_WindowSize packets at a time
		if (m_windowUsed >= m_windowSize)
			break;

		if( !_ftp->BlockDone( blockNumber ) && _ftp->CanSendBlock( blockNumber ) )
		{			
			Directory *letter = _ftp->MakeBlockSendLetter( blockNumber );
			_lettersToSend.PutDataAtEnd( letter );
			m_windowUsed++;
			numPacketsSent++;
		}
	}

	return numPacketsSent;
}


int FTPManager::MakeSendLettersAck( AckSet *_ackSet, LList<Directory *> &_lettersToSend )
{
	int numBlocks = _ackSet->NumBlocks();

	const int maxNumBlocksToAckInOnePacket = 256;
	int blockNumbers[maxNumBlocksToAckInOnePacket];
	int numBlocksAcked = 0;
	int totalNumBlocksAcked = 0;
	bool atLeastOneUrgent = false;

	double now = GetHighResTime();

	m_lastAckTime = now;

	while( true )
	{
		double latestBlockReceived = 0.0;

		for (int blockNumber = 0; blockNumber < numBlocks; blockNumber++)
		{
			double receivedTime;
			int acksRemaining = _ackSet->AckBlock( blockNumber, receivedTime );

			if( acksRemaining  )
			{
				blockNumbers[numBlocksAcked++] = blockNumber;
				totalNumBlocksAcked++;
			}

			if( acksRemaining == NUM_ACKS_TO_SEND )
				atLeastOneUrgent = true;

			if(	latestBlockReceived < receivedTime )
				latestBlockReceived = receivedTime;

			if( numBlocksAcked == maxNumBlocksToAckInOnePacket )
				break;
		}

		if( numBlocksAcked == 0 )
			break;

		// Don't send the ACK unless it's time to do so, or if
		// we've just received a data packet.
		if( !atLeastOneUrgent && now < m_lastAckTime + m_roundTripTime )
			break;

		m_lastAckTime = now;

		Directory *letter = new Directory;
		
		letter->SetName( NET_DARWINIA_MESSAGE );
		letter->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_FILESEND );
		letter->CreateData( NET_DARWINIA_SEQID, -1 );
		letter->CreateData( NET_DARWINIA_FILENAME, _ackSet->Name() );
		letter->CreateData( NET_DARWINIA_BLOCKNUMBER, blockNumbers, numBlocksAcked );
		
		_lettersToSend.PutDataAtEnd( letter );

		// Cycle round to see if there are any other packets to ack
		numBlocksAcked = 0;
	}

	return totalNumBlocksAcked;
}


bool FTPManager::ReceiveLetter( Directory *_letter )
{
	const char *command = _letter->GetDataString( NET_DARWINIA_COMMAND );

	if (command && strcmp( command, NET_DARWINIA_FILESEND ) == 0)
	{
		if( _letter->HasData( NET_DARWINIA_BLOCKDATA ) )
		{
			ReceiveLetterData( _letter );
		}
		else
		{
			ReceiveLetterAck( _letter );
		}

		return true;
	}

	return false;
}


void FTPManager::ReceiveLetterData( Directory *_letter )
{
	const char *filename = _letter->GetDataString( NET_DARWINIA_FILENAME );
	int fileSize = _letter->GetDataInt( NET_DARWINIA_FILESIZE );
	int blockNumber = _letter->GetDataInt( NET_DARWINIA_BLOCKNUMBER );
	int blockSize = 0;
	const char *blockData = (const char *) _letter->GetDataVoid( NET_DARWINIA_BLOCKDATA, &blockSize );
	
	AckSet *ackSet = m_ackSets.GetData( filename );
	if( !ackSet )
	{
		ackSet = new AckSet( filename );
		m_ackSets.PutData( filename, ackSet );
	}
	ackSet->MarkAckToSend( blockNumber );


	FTP *ftp = m_receiving.GetData( filename );
	if( !ftp ) 
	{
		if( m_finished.GetData( filename ) )
		{
#ifdef FTP_MANAGER_VERBOSE_DEBUG
			AppDebugOut( "FTPManager[%s]: Discarding duplicate transmission of file %s\n", m_name, filename );
#endif
			return;
		}

		// New file
		ftp = new FTP( filename, NULL, fileSize );
		m_receiving.PutData( filename, ftp );

		AppDebugOut( "FTPManager[%s]: Receiving file %s (%d bytes, %d blocks)\n", m_name, filename, fileSize, ftp->NumBlocks() );
	}

	if( !ftp->BlockDone( blockNumber ) )
	{
		char *data = ftp->BlockData( blockNumber );
		if( data && blockSize == ftp->BlockSize( blockNumber ) )
		{
			memcpy( data, blockData, blockSize );

			ftp->MarkBlockDone( blockNumber );

#ifdef FTP_MANAGER_VERBOSE_DEBUG
			AppDebugOut( "FTPManager[%s]: Received block %d/%d for file %s\n", m_name, blockNumber, ftp->NumBlocks(), filename );
#endif

			if (ftp->Complete())
				AppDebugOut( "FTPManager[%s]: Finished receiving file %s\n", m_name, filename );
		}
	}
}


void FTPManager::ReceiveLetterAck( Directory *_letter )
{
	const char *filename = _letter->GetDataString( NET_DARWINIA_FILENAME );
	FTP *ftp = m_sending.GetData( filename );
	double now = GetHighResTime();

	if( ftp )
	{
		unsigned numBlocks = 0;
		int *blockNumbers = _letter->GetDataInts( NET_DARWINIA_BLOCKNUMBER, &numBlocks );
		double maxRoundTripTime = 0.0;

#ifdef FTP_MANAGER_VERBOSE_DEBUG
		AppDebugOut( "FTPManager[%s]: Received acks for file %s, blocks: ", m_name, filename );
#endif 

		for( unsigned i = 0; i < numBlocks; i++ )
		{
			if( ftp->MarkBlockDone( blockNumbers[i] ) )
			{
				m_windowUsed--;
				// For every packet successfully received, increase the window size
				m_windowSize++;
			}

			double roundTripTime = now - ftp->GetBlockSendTime( blockNumbers[i] );

			if (roundTripTime > maxRoundTripTime)			
				maxRoundTripTime = roundTripTime;

#ifdef FTP_MANAGER_VERBOSE_DEBUG
			AppDebugOut("%d ", blockNumbers[i]);
#endif 
		}
#ifdef FTP_MANAGER_VERBOSE_DEBUG
		AppDebugOut( "\n" );
#endif 

		if( ftp->Complete() )
		{
			AppDebugOut( "FTPManager[%s]: Finished sending %s\n", m_name, filename );
			m_sending.RemoveData( filename );
			delete ftp;
		}

		if( numBlocks > 0 )
		{
#ifdef FTP_MANAGER_VERBOSE_DEBUG
			AppDebugOut( "FTPManager[%s]: Round trip time for file %s = %f\n", m_name, filename, maxRoundTripTime );
#endif 
			m_roundTripTime = 0.9 * m_roundTripTime + 0.1 * maxRoundTripTime;
		}
	}
}


FTP *FTPManager::Retrieve( const char *_filename )
{
	FTP *result = m_receiving.GetData( _filename );
	if( result && result->Complete() )
	{
		m_finished.PutData( _filename, true );
		m_receiving.RemoveData( _filename );
		return result;
	}

	return NULL;
}


void FTPManager::RetrieveAll( LList<FTP *> &_list )
{
	int size = m_receiving.Size();
	for (int i = 0; i < size; i++)
	{
		if( m_receiving.ValidIndex( i ) )
		{
			FTP *ftp = m_receiving[i];
			if( ftp->Complete() )
			{
				_list.PutDataAtEnd( ftp );
				m_finished.PutData( ftp->Name(), true );
				m_receiving.MarkNotUsed( i );
			}
		}
	}
}


void FTPManager::Reset()
{
	m_sending.EmptyAndDelete();
	m_receiving.EmptyAndDelete();
	m_finished.Empty();
}


void FTPManager::Lock()
{
	m_mutex->Lock();
}


void FTPManager::Unlock()
{
	m_mutex->Unlock();
}
