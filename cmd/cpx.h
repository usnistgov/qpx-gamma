#include "engine.h"

struct CpxLine {
  std::string command;
  std::vector<std::string> params;
};

bool parse_file(std::ifstream&, std::list<CpxLine> &, std::vector<std::string> &);

class Cpx {
public:
  Cpx() : engine_(Qpx::Engine::getInstance())
  {
    interruptor_.store(false);
  }
  
  bool interpret(std::list<CpxLine> commands, std::vector<double> variables);
  
private:
  bool boot(std::vector<std::string> &tokens);
  bool templates(std::vector<std::string> &tokens);
  bool run_mca(std::vector<std::string> &tokens);
  bool save_qpx(std::vector<std::string> &tokens);

  Qpx::Project   spectra_;
  Qpx::Engine       &engine_;
  boost::atomic<bool> interruptor_;
};
