#include <sys/time.h>
#include <iostream>

class timer
{
  public:
    void start() {
      gettimeofday(&m_start,NULL);
    }
    void stop() {
      gettimeofday(&m_stop,NULL);
    }
    void print(){
      timersub(&m_stop,&m_start,&m_result);
      std::cout << "time: " << ((float)m_result.tv_sec + (float)m_result.tv_usec/1000000.0) << std::endl;
    }
    
  private:
    timeval m_start;
    timeval m_stop;
    timeval m_result;
    
};
    

