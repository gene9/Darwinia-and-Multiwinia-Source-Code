#include "rarbloat.h"
#include "smallfn.cpp"


int main(int argc, char *argv[])
{
	setbuf(stdout,NULL);
	ErrHandler.SetSignalHandlers(true);

	RARInitData();

	SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

	CommandData Cmd;

	if (Cmd.IsConfigEnabled(argc,argv))
	{
		Cmd.ReadConfig(argc,argv);	// Read .rarrc or other config file if specified
		Cmd.ParseEnvVar();			// Parse switches from environment variables
	}
	for (int I=1;I<argc;I++)
	{
		Cmd.ParseArg(argv[I],NULL);
	}

	Cmd.ParseDone();

	InitConsoleOptions(Cmd.MsgStream,Cmd.Sound);
	InitSystemOptions(Cmd.SleepTime);
	InitLogOptions(Cmd.LogName);
	ErrHandler.SetSilent(Cmd.AllYes || Cmd.MsgStream==MSG_NULL);
	ErrHandler.SetShutdown(Cmd.Shutdown);

	Cmd.OutTitle();
	Cmd.ProcessCommand();

	File::RemoveCreated();

	return(ErrHandler.GetErrorCode());
}
