#include "simulator.h"
#include "wrapper.h"

struct CpxLine {
  std::string command;
  std::vector<std::string> params;
};

bool parse_file(std::ifstream&, std::list<CpxLine> &, std::vector<std::string> &);

class Cpx {
public:
  Cpx() : pixie_(Pixie::Wrapper::getInstance())
  {
    interruptor_.store(false);
  }
  
  bool interpret(std::list<CpxLine> commands, std::vector<double> variables);
  
private:
  bool boot(std::vector<std::string> &tokens);
  bool load_simulation(std::vector<std::string> &tokens);
  bool templates(std::vector<std::string> &tokens);
  bool run_simulation(std::vector<std::string> &tokens);
  bool run_mca(std::vector<std::string> &tokens);
  bool save_qpx(std::vector<std::string> &tokens);
  bool set_mod(std::vector<std::string> &tokens);
  bool set_chan(std::vector<std::string> &tokens);

  Pixie::SpectraSet   spectra_;
  Pixie::Simulator    source_;
  Pixie::Wrapper      &pixie_;
  boost::atomic<bool> interruptor_;
};
