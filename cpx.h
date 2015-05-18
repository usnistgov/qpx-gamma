#include "simulator.h"
#include "wrapper.h"

class Cpx {
public:
  Cpx() : pixie_(Pixie::Wrapper::getInstance())
  {
    interruptor_.store(false);
  }

  bool interpret(std::string command, std::vector<std::string> &tokens);
  
private:
  bool boot(std::vector<std::string> &tokens);
  bool load_simulation(std::vector<std::string> &tokens);
  bool templates(std::vector<std::string> &tokens);
  bool run_simulation(std::vector<std::string> &tokens);
  bool run_mca(std::vector<std::string> &tokens);
  bool save_qpx(std::vector<std::string> &tokens);

  Pixie::SpectraSet   spectra_;
  Pixie::Simulator    source_;
  Pixie::Wrapper      &pixie_;
  boost::atomic<bool> interruptor_;
};
