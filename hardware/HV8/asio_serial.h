/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      AsioSerial
 *
 ******************************************************************************/

#ifndef ASIO_SERIAL
#define ASIO_SERIAL

#include <string>
#include <boost/asio.hpp>

enum flush_type
{
  flush_receive = TCIFLUSH,
  flush_send = TCIOFLUSH,
  flush_both = TCIOFLUSH
};

class Serial
{
public:
 Serial();
 ~Serial();
 
 void open(const std::string& portname, 
           int baudrate, 
           int charactersize, 
           boost::asio::serial_port_base::parity::type parity, 
           boost::asio::serial_port_base::stop_bits::type stopbits,
           boost::asio::serial_port_base::flow_control::type flowcontrol);
  
 void close();
  
 size_t receive(void* data, size_t length);
 size_t send(void* data, size_t length);

 void writeString(std::string s);
 std::string readLine(int timeout);

 void flush(flush_type);

private:
 boost::asio::io_service m_io;
 boost::asio::serial_port m_serialPort;
};

#endif
