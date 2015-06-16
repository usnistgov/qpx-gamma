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

  std::vector<std::string> cmd_params;
  for (int i=2; i<argc; ++i)
    cmd_params.push_back(std::string(argv[i]));
  
  std::list<CpxLine> program;

  if (!parse_file(gabfile, program, cmd_params)) {
    PL_ERR << "<cpx> parsing failed. Aborting";
    return 1;
  }

  Cpx interpreter;
  std::vector<double> variables;
  if (!interpreter.interpret(program, variables)) {
    PL_ERR << "<cpx> interpreting failed. Aborting";
    return 1;
  }

  return 0;
}

bool parse_file(std::ifstream &file, std::list<CpxLine> &program, std::vector<std::string> &cmd_params) {
   std::list<std::string> lines;

  //read whole file, omitting commented and empty lines
  while (!file.eof())
  {
    char buf[MAX_CHARS_PER_LINE];
    file.getline(buf, MAX_CHARS_PER_LINE);
    if (buf[0] != '#') {
      std::string buf_str(buf);
      boost::algorithm::trim(buf_str);
      if (buf_str.size() > 0)
        lines.push_back(buf_str);
    }
  }

  while (!lines.empty()) {
    //tokenize
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, lines.front(), boost::algorithm::is_any_of(" "));

    //populate parameters, replacing $x with command line arguments
    CpxLine line;
    line.command = tokens[0];
    for (int j=1; j<tokens.size(); ++j) {
      if (tokens[j][0] == '$') {
        int parnr = boost::lexical_cast<int>(tokens[j].substr(1,std::string::npos)) - 1;
        if ((parnr > -1) && (parnr < cmd_params.size()))
          line.params.push_back(cmd_params[parnr]);
        else {
          PL_ERR << "<cpx> command line option not provided for token " << tokens[j]
                 << " for line: " << lines.front();
          return false;
        }
      } else
        line.params.push_back(tokens[j]);
    }
    lines.pop_front();
    program.push_back(line);
  }
  return true;
}


bool Cpx::interpret(std::list<CpxLine> commands, std::vector<double> variables) {
  bool running = true;

  while (running && !commands.empty()) {
    CpxLine line = commands.front();
    commands.pop_front();

    for (int i=0; i<line.params.size(); ++i) {
      if (line.params[i][0] == '%') {
        int varnr = boost::lexical_cast<int>(line.params[i].substr(1,std::string::npos));
        int j = varnr;

        if (varnr < 0)
          j = 0 - varnr - 1;
        else if (varnr > 0)
          j = varnr - 1;
        else {
          PL_ERR << "<cpx> no variable 0";
          return false;
        }
          
        
        if (j < variables.size()) {
          std::string val = std::to_string(variables[j]);
          if (varnr > 0)
            line.params[i] = val;
          else if ((varnr < 0) && (i > 0))
            line.params[i-1] += val;
          else {
            PL_ERR << "<cpx> cannot concatenate below token 0";
            return false;
          }
          
          
        } else {
          PL_ERR << "<cpx> no variable " << j
                 << " in  this scope";
          return false;
        }
      }
    }

    PL_INFO << "<cpx> interpreting " << line.command;
    boost::this_thread::sleep(boost::posix_time::seconds(2));
    
    bool success=false;
    if (line.command == "end") {
      PL_INFO << "<cpx> exiting";
      return true;      
    }
    else if (line.command == "boot")
      success = boot(line.params);
    else if (line.command == "simulation")
      success = load_simulation(line.params);
    else if (line.command == "templates")
      success = templates(line.params);
    else if (line.command == "run_simulation")
      success = run_simulation(line.params);
    else if (line.command == "run_mca")
      success = run_mca(line.params);
    else if (line.command == "save_qpx")
      success = save_qpx(line.params);
    else if (line.command == "set_mod")
      success = set_mod(line.params);
    else if (line.command == "set_chan")
      success = set_chan(line.params);
    else if (line.command == "endfor") {
      if (variables.size())
        return true;
      else {
        PL_ERR << "<cpx> not inside loop";
        return false;
      }
    }
    else if (line.command == "for") {
      if (line.params.size() < 3) {
        PL_ERR << "<cpx> not enough parameters provided for FOR";
        return false;
      }
      double start = boost::lexical_cast<double>(line.params[0]);
      double step = boost::lexical_cast<double>(line.params[1]);
      double end = boost::lexical_cast<double>(line.params[2]);

      variables.push_back(0.0);
      for (double d=start; d <= end; d +=step) {
        variables[variables.size() - 1] = d;
        bool ret =  interpret(commands, variables);
        if (!ret)
          return false;
      }
      success = true;
      variables.pop_back();
      while (!commands.empty() && (commands.front().command != "endfor"))
        commands.pop_front();
      if (!commands.empty() && (commands.front().command == "endfor"))
        commands.pop_front();
    }

    if (!success)
      return false;
    
  }
  return true;
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

bool Cpx::set_chan(std::vector<std::string> &tokens) {
  if (tokens.size() < 3) {
    PL_ERR << "<cpx> expected syntax: set_chan chan_number SETTING_NAME value";
    return false;
  }
  int channum = boost::lexical_cast<int>(tokens[0]);
  std::string setting_name(tokens[1]);
  double value = boost::lexical_cast<double>(tokens[2]);

  pixie_.settings().set_chan(setting_name, value, Pixie::Channel(channum));
  return true;
}

bool Cpx::set_mod(std::vector<std::string> &tokens) {
  if (tokens.size() < 2) {
    PL_ERR << "<cpx> expected syntax: set_chan SETTING_NAME value";
    return false;
  }
  std::string setting_name(tokens[0]);
  double value = boost::lexical_cast<double>(tokens[1]);

  pixie_.settings().set_mod(setting_name, value);
  return true;
}

bool Cpx::boot(std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
    PL_ERR << "<cpx> expected syntax: boot /home/user/qpxdata";
    return false;
  }
  std::string data_dir(tokens[0]);
  //defaults
  std::vector<std::string> boot_files_(7);
  boot_files_[0] = data_dir + "/XIA/Firmware/FippiP500.bin";
  boot_files_[1] = data_dir + "/XIA/Firmware/syspixie_revC.bin";
  boot_files_[2] = data_dir + "/XIA/Firmware/pixie.bin";
  boot_files_[3] = data_dir + "/XIA/DSP/PXIcode.bin";
  boot_files_[4] = data_dir + "/XIA/Configuration/HPGe.set";
  boot_files_[5] = data_dir + "/XIA/DSP/PXIcode.var";
  boot_files_[6] = data_dir + "/XIA/DSP/PXIcode.lst";
  std::vector<uint8_t> boot_slots_(1);
  boot_slots_[0] = 2;
  std::vector<std::string> default_detectors_(4);
  default_detectors_[0] = "Sophia";
  default_detectors_[1] = "Daphne";
  default_detectors_[2] = "N/A";
  default_detectors_[3] = "N/A";

  //defaults
  XMLableDB<Pixie::Detector> detectors_("Detectors");
  detectors_.read_xml(data_dir + "/default_detectors.det");
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
  boost::this_thread::sleep(boost::posix_time::seconds(1));
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
