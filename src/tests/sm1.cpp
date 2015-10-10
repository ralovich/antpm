// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2011-2014 RALOVICH, Kristóf                            //
//                                                                      //
// This program is free software; you can redistribute it and/or modify //
// it under the terms of the GNU General Public License as published by //
// the Free Software Foundation; either version 3 of the License, or    //
// (at your option) any later version.                                  //
//                                                                      //
// This program is distributed in the hope that it will be useful,      //
// but WITHOUT ANY WARRANTY; without even the implied warranty of       //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        //
// GNU General Public License for more details.                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
// ***** END LICENSE BLOCK *****

// create dummy serial port using UDS
// run AntFr310XT on that port
// in return just send ANT replies



#include "common.hpp"

#include <iostream>
#include <string>
#include <cctype>
#include <AntFr310XT.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>

#define BOOST_TEST_MODULE sm1
//#include <boost/test/included/unit_test.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace antpm;
//namespace fs = boost::filesystem;

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

using boost::asio::local::stream_protocol;

class AntHostMessenger
{
public:
  AntHostMessenger(boost::asio::io_service& io_service)
    : socket_(io_service)
  {
  }

  stream_protocol::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    // Wait for request.
    socket_.async_read_some(boost::asio::buffer(data_),
        boost::bind(&AntHostMessenger::handle_read,
          this, boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

private:
  void handle_read(const boost::system::error_code& ec, std::size_t size)
  {
    if (!ec)
    {
      // Compute result.
      for (std::size_t i = 0; i < size; ++i)
        data_[i] = std::toupper(data_[i]);

      // Send result.
      boost::asio::async_write(socket_, boost::asio::buffer(data_, size),
          boost::bind(&AntHostMessenger::handle_write,
            this, boost::asio::placeholders::error));
    }
    else
    {
      throw boost::system::system_error(ec);
    }
  }

  void handle_write(const boost::system::error_code& ec)
  {
    if (!ec)
    {
      // Wait for request.
      socket_.async_read_some(boost::asio::buffer(data_),
          boost::bind(&AntHostMessenger::handle_read,
            this, boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      throw boost::system::system_error(ec);
    }
  }

  stream_protocol::socket socket_;
  boost::array<char, 512> data_;
};

void run(boost::asio::io_service* io_service)
{
  try
  {
    std::cout << "io_service=" << io_service << endl;
    io_service->run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception in thread: " << e.what() << "\n";
    std::exit(1);
  }
}

BOOST_AUTO_TEST_CASE(test_asio)
{
  antpm::Log::instance()->addSink(std::cout);
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_DBG3);

  BOOST_CHECK(true);

  try
  {
    //SerialTester st;
    //SerialTester1* st = new SerialTester1();
    //AntFr310XT watch2(st);

    boost::asio::io_service io_service;
    std::cout << "io_service=" << &io_service << endl;

    // Create filter and establish a connection to it.
    AntHostMessenger filter(io_service);
    stream_protocol::socket socket(io_service);
    boost::asio::local::connect_pair(socket, filter.socket());
    filter.start();

    // The io_service runs in a background thread to perform filtering.
    boost::thread bgthread(boost::bind(run, &io_service));

    //for (;;)
    {
      // Collect request from user.
      //std::cout << "Enter a string: ";
      std::string request;
      //std::getline(std::cin, request);
      request = "abcdef";

      // Send request to filter.
      boost::asio::write(socket, boost::asio::buffer(request));

      // Wait for reply from filter.
      std::vector<char> reply(request.size());
      boost::asio::read(socket, boost::asio::buffer(reply));

      // Show reply to user.
      std::cout << "Result: ";
      std::cout.write(&reply[0], request.size());
      std::cout << std::endl;

      BOOST_CHECK(reply.size()==6);
      BOOST_CHECK(reply[0]=='A');

      //break;
    }

    io_service.stop();
    bgthread.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
    std::exit(1);
  }
}


#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
//# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

DEFAULT_LOG_INSTANTIATOR

namespace antpm
{

  class SerialTester0 : public Serial
  {
  public:
    SerialTester0() {}
    virtual ~SerialTester0() {}

    virtual bool open() { return true; }
    virtual void close() { }

    virtual bool read(char* dst, const size_t sizeBytes, size_t& bytesRead) {return false;}
    virtual bool readBlocking(char* dst, const size_t sizeBytes, size_t& bytesRead) {return false;}
    virtual bool write(const char* src, const size_t sizeBytes, size_t& bytesWritten) {return false;}

  private:
    void* ioHandler();

  public:
    virtual const size_t getQueueLength() const { return 0; }
    virtual const char*  getImplName() { return "SerialTester0"; }
    virtual bool         isOpen() const { return true; }
    virtual bool         setWriteDelay(const size_t ms) {return true;}

  };


  //
  class SerialTester1 : public Serial
  {
  public:
    SerialTester1() {}
    virtual ~SerialTester1() {}

    virtual bool open() { return true; }
    virtual void close() { m_q.clear(); }

    virtual bool read(char* dst, const size_t sizeBytes, size_t& bytesRead) {return false;}
    virtual bool readBlocking(char* dst, const size_t sizeBytes, size_t& bytesRead) {return false;}
    virtual bool write(const char* src, const size_t sizeBytes, size_t& bytesWritten)
    {
      bytesWritten = 0;
      for(size_t i = 0; i < sizeBytes; i++)
      {
        m_q.push(src[i]);
        bytesWritten += 1;
      }
      return false;
    }

  private:
    void* ioHandler();

  public:
    virtual const size_t getQueueLength() const { return m_q.size(); }
    virtual const char*  getImplName() { return "SerialTester1"; }
    virtual bool         isOpen() const { return true; }
    virtual bool         setWriteDelay(const size_t ms) {return true;}

  private:
    void queueData();

  private:
    lqueue3<uint8_t> m_q; // FIFO for bytes written into this "serial port" with write() method
    lqueue4<uint8_t> m_q_r; // FIFO for bytes produced by this "serial port", to be emptied by read()
  };


  class SerialTester2 : public Serial
  {
  public:
    SerialTester2() {}
    virtual ~SerialTester2() {}

    virtual bool open() { return true; }
    virtual void close() { m_q.clear(); }

    virtual bool read(char* dst, const size_t sizeBytes, size_t& bytesRead) {return false;}
    virtual bool readBlocking(char* dst, const size_t sizeBytes, size_t& bytesRead) {return false;}
    virtual bool write(const char* src, const size_t sizeBytes, size_t& bytesWritten)
    {
      bytesWritten = 0;
      for(size_t i = 0; i < sizeBytes; i++)
      {
        m_q.push(src[i]);
        bytesWritten += 1;
      }
      return true;
    }

  private:
    void* ioHandler();

  public:
    virtual const size_t getQueueLength() const { return m_q.size(); }
    virtual const char*  getImplName() { return "SerialTester2"; }
    virtual bool         isOpen() const { return true; }
    virtual bool         setWriteDelay(const size_t ms) {return true;}

  private:
    void queueData();

  private:
    lqueue3<uint8_t> m_q; // FIFO for bytes written into this "serial port" with write() method
    lqueue4<uint8_t> m_q_r; // FIFO for bytes produced by this "serial port", to be emptied by read()
  };

}

BOOST_AUTO_TEST_CASE(test_serial0)
{
  antpm::Log::instance()->addSink(std::cout);
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_DBG3);

  Serial* st = new SerialTester0();
  AntFr310XT watch2(st);

  watch2.run();

}

BOOST_AUTO_TEST_CASE(test_serial1)
{
  antpm::Log::instance()->addSink(std::cout);
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_DBG3);

  Serial* st = new SerialTester1();
  AntFr310XT watch2(st);

  watch2.run();

}

BOOST_AUTO_TEST_CASE(test_serial2)
{
  antpm::Log::instance()->addSink(std::cout);
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_DBG3);

  Serial* st = new SerialTester2();
  AntFr310XT watch2(st);

  watch2.run();

}

