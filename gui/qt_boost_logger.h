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
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      LogEmitter - emits logger text to Qt gui
 *      LogStreamBuffer - stream that interfaces boost logger to Qt emitter
 *
 ******************************************************************************/

#pragma once

#include <iostream>
#include <streambuf>
#include <string>

#include <QObject>

class LogEmitter : public QObject
{
Q_OBJECT
public:
     int write(std::string someline) { emit(writeLine(QString::fromStdString(someline))); return 1; }
signals:
     void writeLine(QString someline);
};

class LogStreamBuffer : public std::basic_streambuf<char>
{
public:
    LogStreamBuffer(std::ostream &stream, LogEmitter &emitter) : out_stream_(stream), log_emitter_(emitter)
    {
        old_buffer_ = out_stream_.rdbuf();
        out_stream_.rdbuf(this);
    }
    ~LogStreamBuffer()
    {
        // output anything that remains
        if (!buffer_string_.empty())
            log_emitter_.write(buffer_string_.c_str());
        out_stream_.rdbuf(old_buffer_);
    }
protected:
    virtual int_type overflow(int_type v)
    {
        if (v == '\n')
        {
            log_emitter_.write(buffer_string_.c_str());
            buffer_string_.erase(buffer_string_.begin(), buffer_string_.end());
        }
        else
            buffer_string_ += v;
        return v;
    }

    virtual std::streamsize xsputn(const char *p, std::streamsize n)
    {
        buffer_string_.append(p, p + n);
        std::size_t pos = 0;
        while (pos != std::string::npos)
        {
            pos = buffer_string_.find('\n');
            if (pos != std::string::npos)
            {
                log_emitter_.write(buffer_string_.c_str());
                buffer_string_.erase(buffer_string_.begin(), buffer_string_.begin() + pos + 1);
            }
        }
        return n;
    }

private:
    std::ostream &out_stream_;
    std::streambuf *old_buffer_;
    std::string buffer_string_;
    LogEmitter &log_emitter_;
};
