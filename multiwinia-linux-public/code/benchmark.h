#ifndef _included_benchmark_h
#define _included_benchmark_h



class BenchMark
{
protected:
    char    m_filename[512];
    double  m_timer;
    bool    m_recording;

    LList   <char *> m_dxDiag;

protected:
    char    *BuildTimeIndex();
    char    *BuildBenchmark();
    bool    BuildDxDiag();

public:
    BenchMark();
	~BenchMark();

    void RequestDXDiag();

    void Begin();
    void Advance();
    void End();
    void Upload();
};




#endif
