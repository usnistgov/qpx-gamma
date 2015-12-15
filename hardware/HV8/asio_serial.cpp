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

#include "asio_serial.h"

Serial::Serial() : m_serialPort(m_io) {
 m_io.run();
}
 
Serial::~Serial() {
 m_serialPort.close();

}

void Serial::open(const std::string& portname, 
                  int baudrate, 
                  int charactersize, 
                  boost::asio::serial_port_base::parity::type parity, 
                  boost::asio::serial_port_base::stop_bits::type stopbits,
                  boost::asio::serial_port_base::flow_control::type flowcontrol)
{
 m_serialPort.open(portname.c_str());
 m_serialPort.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
 m_serialPort.set_option(boost::asio::serial_port_base::character_size(charactersize));
 m_serialPort.set_option(boost::asio::serial_port_base::parity(parity));
 m_serialPort.set_option(boost::asio::serial_port_base::stop_bits(stopbits));
 m_serialPort.set_option(boost::asio::serial_port_base::flow_control(flowcontrol));
}
void Serial::close()
{
 m_serialPort.close();
}
size_t Serial::receive(void* data, size_t length)
{
 return m_serialPort.read_some(boost::asio::buffer(data, length));
}
size_t Serial::send(void* data, size_t length)
{
 return m_serialPort.write_some(boost::asio::buffer(data, length));
}

void Serial::flush(flush_type what)
{
  boost::system::error_code error;

  if (0 == ::tcflush(m_serialPort.lowest_layer().native_handle(), what))
  {
    error = boost::system::error_code();
  }
  else
  {
    error = boost::system::error_code(errno,
        boost::asio::error::get_system_category());
  }
}

void Serial::writeString(std::string s)
{
    boost::asio::write(m_serialPort, boost::asio::buffer(s.c_str(),s.size()));
}

std::string Serial::readLine(int timeout)
{
    char c;
    std::string result;
    for(;;)
    {
        boost::asio::read(m_serialPort, boost::asio::buffer(&c,1));
        switch(c)
        {
            case '\r':
                break;
            case '\n':
                return result;
            default:
                result += c;
        }
    }
}

