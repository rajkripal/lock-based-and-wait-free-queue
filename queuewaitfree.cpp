#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <random>
#include <chrono>
#include <vector>

bool enqflag, deqflag;

class WaitFreeQueue {
private:
  volatile long long head, tail;
  int * items;
  long long n;
public:

  WaitFreeQueue(long long capacity) {
    head = 0;
    tail = 0;
    n = capacity;
    items = new int[capacity];
  }

  ~WaitFreeQueue() {
    delete[] items;
  }

  void enq(int x) {
    enqflag = false;
    try {
      if(tail - head == n - 1) {
        throw "Queue is full!!!";
      }
      items[tail%n] = x;
      if (tail == LLONG_MAX) {
        std::cout << "tail exceeds long long limit!!!" << std::endl;
        exit(-1);
      }
      tail += 1;
    } catch (const char* msg) {
      std::cout << msg << std::endl;
      enqflag = true;
    }

  }

  int deq () {
    deqflag = false;
    try {
      if (tail == head) {
        throw "Queue is empty!!!";
      }
      int x = items[head % n];
      if (head == LLONG_MAX) {
        std::cout << "Head exceeds long long limit!!!" << std::endl;
        exit(-1);
      }
      head += 1;
      return x;
    } catch (const char* msg) {
      std::cout << msg << std::endl;
      deqflag = true;
    }
  }
};

std::vector<std::string> list;

struct ThreadMeta {
  int count;
  int l;
  WaitFreeQueue * q;
};

struct BookKeep {
  std::chrono::microseconds invtime;
  std::chrono::microseconds restime;
  bool nilflag;
  int val;
  int count;
};

std::vector <struct BookKeep> enqlist;
std::vector <struct BookKeep> deqlist;

double enqsum = 0;
double deqsum = 0;

std::string suffix (int a) {
  if (a >= 10 && a <= 20) {
    return "th";
  }
  a = a % 10;
  if (a == 1) {
    return "st";
  } else if (a == 2) {
    return "nd";
  } else if (a == 3) {
    return "rd";
  } else {
    return "th";
  }
}

void* enqueue (void* argument) {
  struct ThreadMeta* eqdata = (struct ThreadMeta*) argument;

  double const exp_dist_lambda = eqdata -> l;

  int seed = std::chrono::system_clock::now().time_since_epoch().count();

  std::default_random_engine rg(seed);
  std::exponential_distribution <double> dist(exp_dist_lambda);

  for (int i = 0;  i < eqdata -> count; i++) {
    double t1d = dist(rg);
    int t1 = (useconds_t)(t1d * 1e6);
    int x = rand() % 10000;
   
    std::chrono::microseconds ms1 = std::chrono::duration_cast <std::chrono::microseconds> (std::chrono::system_clock::now().time_since_epoch());
    eqdata -> q -> enq(x);
    std::chrono::microseconds ms2 = std::chrono::duration_cast <std::chrono::microseconds> (std::chrono::system_clock::now().time_since_epoch());
    std::string vecstring = std::to_string(i + 1);
    vecstring = vecstring + suffix(i + 1);
    vecstring = vecstring + " Enq(";
    vecstring = vecstring + std::to_string(x);
    vecstring = vecstring + ").inv at ";
    vecstring = vecstring + std::to_string(ms1.count());
    list.push_back(vecstring);

    vecstring = "";
    vecstring = std::to_string(i + 1);
    vecstring = vecstring + suffix(i + 1);
    vecstring = vecstring + " Enq(";
    if (enqflag) {
      vecstring = vecstring + "nil";
    }else {
      vecstring = vecstring + std::to_string(x);
    }
    vecstring = vecstring + ").rsp at ";
    vecstring = vecstring + std::to_string(ms2.count());
    list.push_back(vecstring);

    struct BookKeep estr;
    estr.invtime = ms1;
    estr.restime = ms2;
    estr.val = x;
    estr.nilflag = enqflag;
    estr.count = i;
    enqlist.push_back(estr);
    enqsum += ms2.count() - ms1.count();
    usleep(t1);
  }
}

void* dequeue (void* argument) {
  struct ThreadMeta* dqdata = (struct ThreadMeta*) argument;
  double const exp_dist_lambda = dqdata -> l;
  int val;

  int seed = std::chrono::system_clock::now().time_since_epoch().count();

  std::default_random_engine rg(seed);
  std::exponential_distribution <double> dist(exp_dist_lambda);

  for (int i = 0;  i < dqdata -> count; i++) {
    double t1d = dist(rg);
    int t1 = (useconds_t)(t1d * 1e6);
    std::chrono::microseconds ms1 = std::chrono::duration_cast <std::chrono::microseconds> (std::chrono::system_clock::now().time_since_epoch());
    val = dqdata -> q -> deq();
    std::chrono::microseconds ms2 = std::chrono::duration_cast <std::chrono::microseconds> (std::chrono::system_clock::now().time_since_epoch());
    std::string vecstring = std::to_string(i + 1);
    vecstring = vecstring + suffix(i + 1);
    vecstring = vecstring + " Deq().inv at ";
    vecstring = vecstring + std::to_string(ms1.count());
    list.push_back(vecstring);
    
    vecstring = "";
    vecstring = std::to_string(i + 1);
    vecstring = vecstring + suffix(i + 1);
    vecstring = vecstring + " Deq(";
    if (deqflag) {
      vecstring = vecstring + "nil";
    } else {
      vecstring = vecstring + std::to_string(val);  
    }
    vecstring = vecstring + ").rsp at ";
    vecstring = vecstring + std::to_string(ms2.count());
    list.push_back(vecstring);
    
    struct BookKeep dstr;
    dstr.invtime = ms1;
    dstr.restime = ms2;
    dstr.val = val;
    dstr.nilflag = deqflag;
    dstr.count = i;
    deqlist.push_back(dstr);
    deqsum += ms2.count() - ms1.count();
    usleep(t1);
  }
}

int main () {
  std::ifstream ifile;
  ifile.open("inp-params.txt");
  long long N;
  int ecount, dcount, l1, l2;

  while(!ifile.eof()) {
    ifile >> N >> ecount >> dcount >> l1 >> l2;
  }
  ifile.close();

  WaitFreeQueue * q = new WaitFreeQueue (N);
  pthread_t enqueuer, dequeuer; //the threads
  struct ThreadMeta enqMeta, deqMeta;
  enqMeta.count = ecount;
  enqMeta.l = l1;
  enqMeta.q = q;
  deqMeta.count = dcount;
  deqMeta.l = l2;
  deqMeta.q = q;
  int check = pthread_create (&enqueuer, NULL, enqueue, (void*)&enqMeta);
  if (check) {
    std::cout << "Error in creating enqueuer" << std::endl;
  }
  int check2 = pthread_create (&dequeuer, NULL, dequeue, (void*)&deqMeta);
  if (check2) {
    std::cout << "Error in creating dequeuer" << std::endl;
  }

  pthread_join (enqueuer, NULL);
  pthread_join (dequeuer, NULL);
  
  std::ofstream ofile;
  ofile.open("output.txt");
  for (std::vector<std::string>::iterator it = list.begin(); it != list.end(); ++it) {
    ofile << *it << std::endl;
  }
  delete q;
  ofile.close();

  std::ofstream outfile;
  outfile.open("serial.txt");
  while (enqlist.size() > 0 && deqlist.size() > 0) {
    if (enqlist[0].restime.count() < deqlist[0].invtime.count()) {

      outfile << enqlist[0].count + 1 << suffix(enqlist[0].count + 1) << " Enq(";
      
      if (enqlist[0].nilflag) {
        outfile << "nil";
      }else {
        outfile << enqlist[0].val;
      }

      outfile << ").inv" << std::endl;

      outfile << enqlist[0].count + 1 << suffix(enqlist[0].count + 1) << " Enq(";
      
      if (enqlist[0].nilflag) {
        outfile << "nil";
      } else {
        outfile << enqlist[0].val;
      }

      outfile << ").res" << std::endl;

      enqlist.erase(enqlist.begin());
    } else if (deqlist[0].restime.count() < enqlist[0].invtime.count()) {

      outfile << deqlist[0].count + 1 << suffix(deqlist[0].count + 1) << " Deq(";
      if (deqlist[0].nilflag) {
        outfile << "nil";
      } else {
        outfile << deqlist[0].val;
      }
      outfile << ").inv" << std::endl;
      outfile << deqlist[0].count + 1 << suffix(deqlist[0].count + 1) << " Deq(";
      
      if (deqlist[0].nilflag) {
        outfile << "nil";
      } else {
        outfile << deqlist[0].val;
      }

      outfile << ").res" << std::endl;

      deqlist.erase(deqlist.begin());
    } else {
      if (enqlist[0].val == deqlist[0].val) {
        outfile << enqlist[0].count + 1 << suffix(enqlist[0].count + 1) << " Enq(";
        
        if (enqlist[0].nilflag) {
          outfile << "nil";
        }else {
          outfile << enqlist[0].val;
        }

        outfile << ").inv" << std::endl;

        outfile << enqlist[0].count + 1 << suffix(enqlist[0].count + 1) << " Enq(";
        
        if (enqlist[0].nilflag) {
          outfile << "nil";
        } else {
          outfile << enqlist[0].val;
        }

        outfile << ").res" << std::endl;

        enqlist.erase(enqlist.begin());

        outfile << deqlist[0].count + 1 << suffix(deqlist[0].count + 1) << " Deq(";
        if (deqlist[0].nilflag) {
          outfile << "nil";
        } else {
          outfile << deqlist[0].val;
        }
        outfile << ").inv" << std::endl;
        outfile << deqlist[0].count + 1 << suffix(deqlist[0].count + 1) << " Deq(";
        
        if (deqlist[0].nilflag) {
          outfile << "nil";
        } else {
          outfile << deqlist[0].val;
        }

        outfile << ").res" << std::endl;

        deqlist.erase(deqlist.begin());

      } else {
        outfile << deqlist[0].count + 1 << suffix(deqlist[0].count + 1) << " Deq(";
        if (deqlist[0].nilflag) {
          outfile << "nil";
        } else {
          outfile << deqlist[0].val;
        }
        outfile << ").inv" << std::endl;
        outfile << deqlist[0].count + 1 << suffix(deqlist[0].count + 1) << " Deq(";
        
        if (deqlist[0].nilflag) {
          outfile << "nil";
        } else {
          outfile << deqlist[0].val;
        }

        outfile << ").res" << std::endl;

        deqlist.erase(deqlist.begin());
      }
    }
  }

  if (enqlist.size() == 0) {
    while(deqlist.size() > 0) {
      outfile << deqlist[0].count + 1 << suffix(deqlist[0].count + 1) << " Deq(";
      if (deqlist[0].nilflag) {
        outfile << "nil";
      } else {
        outfile << deqlist[0].val;
      }
      outfile << ").inv" << std::endl;
      outfile << deqlist[0].count + 1 << suffix(deqlist[0].count + 1) << " Deq(";
      
      if (deqlist[0].nilflag) {
        outfile << "nil";
      } else {
        outfile << deqlist[0].val;
      }

      outfile << ").res" << std::endl;

      deqlist.erase(deqlist.begin());
    }
  }
  if (deqlist.size() == 0) {
    while (enqlist.size() > 0) {
      outfile << enqlist[0].count + 1 << suffix(enqlist[0].count + 1) << " Enq(";
      
      if (enqlist[0].nilflag) {
        outfile << "nil";
      }else {
        outfile << enqlist[0].val;
      }

      outfile << ").inv" << std::endl;

      outfile << enqlist[0].count + 1 << suffix(enqlist[0].count + 1) << " Enq(";
      
      if (enqlist[0].nilflag) {
        outfile << "nil";
      } else {
        outfile << enqlist[0].val;
      }

      outfile << ").res" << std::endl;

      enqlist.erase(enqlist.begin());
    }
  }

  outfile.close();

  deqsum = deqsum / (dcount * 1.0);
  enqsum = enqsum / (ecount * 1.0);
  std::cout << "Average time to enqueue: " << enqsum << "useconds" << std::endl;
  std::cout << "Average time to dequeue: " << deqsum << "useconds" << std::endl;
}
