#ifndef __FTP_MANAGER_H
#define __FTP_MANAGER_H

class Directory;

#include "lib/tosser/llist.h"
#include "lib/tosser/hash_table.h"

class FTP 
{
public:
	FTP( const char *_filename, const char *_data, int _size );
	~FTP();

	const char *Name() const;
	int Size() const;
	
	int NumBlocks() const;
	int BlockSize( int _blockNumber ) const;
	char *BlockData( int _blockNumber );
	char *Data();

	bool BlockDone( int _blockNumber ) const;
	bool MarkBlockDone( int _blockNumber );
	double GetBlockSendTime( int _blockNumber ) const;

	bool Complete() const;

	bool CanSendBlock( int _blockNumber ) const;
	Directory *MakeBlockSendLetter( int _blockNumber );
	int TimeOutBlocks( double _roundTripTime );

	
private:
	char *m_filename;
	char *m_data;
	
	struct BlockInfo {
		BlockInfo()
			: m_sendTime( -1.0 ), m_done( false ), m_timedOut( true )
		{
		}

		double m_sendTime;
		bool m_done;
		bool m_timedOut;
	};

	BlockInfo *m_blockInfo;
	int m_numBlocksRemaining;
	int m_size;
	int m_numBlocks;
};

class AckSet 
{
public:
	AckSet( const char *_filename );
	~AckSet();

	void MarkAckToSend( int _blockNumber );
	int AckBlock( int _blockNumber, double &_receivedTime );

	const char *Name() const;
	int NumBlocks() const;

	double LastActive() const;

private:
	char *m_filename;

	struct BlockInfo {
		BlockInfo()
			: m_acksRemaining( 0 ), 
			  m_timeReceived( 0.0 )
		{
		}

		int m_acksRemaining;
		double m_timeReceived;
	};

	BlockInfo *m_blockInfo;
	int m_numBlocks;
	double m_lastActive;
};


class NetMutex;

class FTPManager 
{
public:
	FTPManager( const char *_name );
	~FTPManager();

	void SendFile( const char *_filename, const char *_data, int _size );
	bool ReceiveLetter( Directory *_letter );
	int MakeSendLetters( LList<Directory *> &_lettersToSend );
	
	// Once a file is complete it may be retrieved.
	FTP *Retrieve( const char *_filename );
	void RetrieveAll( LList<FTP *> &_list );

	// Cancel all transfers
	void Reset();

	// For ThreadSaftey
	void Lock();
	void Unlock();

private:
	int MakeSendLettersData( FTP *_ftp, LList<Directory *> &_lettersToSend );
	int MakeSendLettersAck( AckSet *_ftp, LList<Directory *> &_lettersToSend );

	void ReceiveLetterData( Directory *_letter );
	void ReceiveLetterAck( Directory *_letter );

private:
	char *m_name;
	int m_windowSize, m_windowUsed;
	double m_roundTripTime;
	double m_lastAckTime;

	HashTable<FTP *> m_sending;
	HashTable<FTP *> m_receiving;
	HashTable<AckSet *> m_ackSets;
	HashTable<bool> m_finished;

	NetMutex *m_mutex;
};

#endif