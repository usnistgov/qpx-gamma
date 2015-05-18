#include "custom_logger.h"
#include "wrapper.h"
#include "cpx.h"

const int MAX_CHARS_PER_LINE = 512;
const int MAX_TOKENS_PER_LINE = 20;
const char* const DELIMITER = " ";

int main(int argc, char *argv[])
{
  //user input
  if (argc < 2) {
    std::cout << "Usage: cpx gamma_acquisition_batch.gab\n";
    return 1;
  }

  std::string gab(argv[1]);
  if (gab.empty()) {
    std::cout << "Bad batch file\n";
    return 1;
  }

  std::ifstream gabfile;
  gabfile.open(gab);
  if (!gabfile.good()) {
    std::cout << "Could not open batch file\n";
    return 1;
  }
  
  CustomLogger::initLogger(nullptr, "qpx_%N.log");
  PL_INFO << "--==cpx console tool for gamma acquisition==--";
  Cpx interpreter;

  std::vector<std::string> script;

  //read whole file, omitting commented and empty lines
  while (!gabfile.eof())
  {
    char buf[MAX_CHARS_PER_LINE];
    gabfile.getline(buf, MAX_CHARS_PER_LINE);
    if (buf[0] != '#') {
      std::string buf_str(buf);
      if (buf_str.size() > 0)
        script.push_back(buf_str);
    }
  }

  //interpret valid lines
  for (int i=0; i < script.size(); ++i) {
    PL_INFO << "<cpx> interpreting: " << script[i];

    std::vector<std::string> tokens;

    boost::algorithm::split(tokens, script[i],
                            boost::algorithm::is_any_of(" "));

    std::string command = tokens[0];
    std::vector<std::string> params;

    for (int i=1; i<tokens.size(); ++i) {
      if (tokens[i][0] == '$') {
        int parnr = boost::lexical_cast<int>(tokens[i].substr(1,std::string::npos));
        if (parnr && (argc >= (parnr+2)))
          params.push_back(std::string(argv[parnr+1]));
        else
          params.push_back("0");
      } else
        params.push_back(tokens[i]);
    }

    if (!interpreter.interpret(command, params))
          return 1;
  }

}

bool Cpx::interpret(std::string command, std::vector<std::string> &tokens) {
  if (command == "boot")
    return boot();
  else if (command == "simulation")
    return load_simulation(tokens);
  else if (command == "templates")
    return templates(tokens);
  else if (command == "run_simulation")
    return run_simulation(tokens);
  else if (command == "run_mca")
    return run_mca(tokens);
  else if (command == "save_qpx")
    return save_qpx(tokens);
}


bool Cpx::load_simulation(std::vector<std::string> &tokens) {
  if (tokens.size() < 3) {
    PL_ERR << "<cpx> expected syntax: simulation source.qpx source_bits matrix_bits";
    return false;
  }
  std::string source(tokens[0]);
  uint16_t source_res = boost::lexical_cast<uint16_t>(tokens[1]);
  uint16_t matrix_res = boost::lexical_cast<uint16_t>(tokens[2]);
  
  PL_INFO << "<cpx> loading simulation from " << source;
  Pixie::SpectraSet temp_spectra;
  temp_spectra.read_xml(source, true);
  source_ = Pixie::Simulator(&temp_spectra, std::array<int,2>({0,1}), source_res, matrix_res);
  return true;
}

bool Cpx::templates(std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
    PL_ERR << "<cpx> expected syntax: template template_file.tem";
    return false;
  }
  std::string file(tokens[0]);

  XMLableDB<Pixie::Spectrum::Template>  spectra_templates_("SpectrumTemplates");
  spectra_templates_.read_xml(file);
  if (spectra_templates_.empty()) {
    PL_ERR << "<cpx> bad template file " << file;
    return false;
  }
  PL_INFO << "<cpx> loading templates from " << file;
  spectra_.clear();
  spectra_.set_spectra(spectra_templates_);
  return true;
}

bool Cpx::run_simulation(std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
    PL_ERR << "<cpx> expected syntax: run_simulation duration";
    return false;
  }
  uint64_t duration = boost::lexical_cast<uint64_t>(tokens[0]);
  if (duration == 0) {
    PL_ERR << "<cpx> bad duration";
    return false;
  }
  
  PL_INFO << "<cpx> running simulation for " << duration;
  pixie_.getFakeMca(source_, spectra_, duration, interruptor_);
  return true;
}

bool Cpx::run_mca(std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
    PL_ERR << "<cpx> expected syntax: run_mca duration [type=3]";
    return false;
  }

  uint64_t duration = boost::lexical_cast<uint64_t>(tokens[0]);
  if (duration == 0) {
    PL_ERR << "<cpx> bad duration";
    return false;
  }
  
  int type_n = 3;
  if (tokens.size() >= 2)
    type_n = boost::lexical_cast<int>(tokens[1]);
  if ((type_n < 0) || (type_n > 3)) {
    PL_ERR << "<cpx> bad type";
    return false;
  }
  Pixie::RunType type = Pixie::RunType(0x100 | type_n);
  
  //double-buffer always
  pixie_.getMca(type, duration, spectra_, interruptor_, true);
  return true;
}

bool Cpx::save_qpx(std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
    PL_ERR << "<cpx> expected syntax: save_qpx filename(.qpx)";
    return false;
  }
  std::string out_name(tokens[0]);
  if (out_name.empty()) {
    //check if exists, if valid
    PL_ERR << "<cpx> bad output file name provided";
    return false;
  }

  std::string full_name = out_name + ".qpx";
  PL_INFO << "<cpx> writing acquired data to " << full_name;
  spectra_.write_xml(full_name);
  return true;
}

bool Cpx::boot() {
  //defaults
  std::vector<std::string> boot_files_(7);
  boot_files_[0] = "/opt/XIA/Firmware/FippiP500.bin";
  boot_files_[1] = "/opt/XIA/Firmware/syspixie_revC.bin";
  boot_files_[2] = "/opt/XIA/Firmware/pixie.bin";
  boot_files_[3] = "/opt/XIA/DSP/PXIcode.bin";
  boot_files_[4] = "/opt/XIA/Configuration/HPGe.set";
  boot_files_[5] = "/opt/XIA/DSP/PXIcode.var";
  boot_files_[6] = "/opt/XIA/DSP/PXIcode.lst";
  std::vector<uint8_t> boot_slots_(1);
  boot_slots_[0] = 2;
  std::vector<std::string> default_detectors_(4);
  default_detectors_[0] = "Sophia";
  default_detectors_[1] = "Daphne";
  default_detectors_[2] = "N/A";
  default_detectors_[3] = "N/A";

  //defaults
  XMLableDB<Pixie::Detector> detectors_("Detectors");
  detectors_.read_xml("/home/liudas/qpxdata/default_detectors.det");
  if (detectors_.empty()) {
    PL_ERR << "<cpx> bad detector db";
    return false;
  }

 //boot
  pixie_.settings().set_boot_files(boot_files_);
  pixie_.settings().set_slots(boot_slots_);
  pixie_.settings().set_sys("OFFLINE_ANALYSIS", 0);
  pixie_.settings().set_sys("KEEP_CW", 1);
  pixie_.settings().set_sys("MAX_NUMBER_MODULES", 7);
  bool success = pixie_.boot();
  bool online = (pixie_.settings().live() == Pixie::LiveStatus::online);
  if (!online) {
      PL_INFO << "<cpx> attempting boot of offline functions";
      pixie_.settings().set_sys("OFFLINE_ANALYSIS", 1);
      success = pixie_.boot();
  }
  if (!success) {
    PL_ERR << "<cpx> couldn't boot";
    return false;
  }

  //apply settings
  //thread sleep 1 sec
  pixie_.settings().set_mod("FILTER_RANGE", 4);
  pixie_.settings().set_mod("ACTUAL_COINCIDENCE_WAIT", 0);
  for (int i =0; i < 4; i++)
    pixie_.settings().set_detector(Pixie::Channel(i), detectors_.get(Pixie::Detector(default_detectors_[i])));
  if (true) { //online
    pixie_.control_adjust_offsets();
    pixie_.settings().load_optimization();
  }
  pixie_.settings().get_all_settings();

  return true;
}
