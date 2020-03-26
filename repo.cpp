#include <ndn-cxx/face.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <iostream>
#include <chrono>
#include <ndn-cxx/util/sha256.hpp>
#include <ndn-cxx/encoding/tlv.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <boost/asio.hpp>
#include <sstream>

// #include "nd-packet-format.h"
#include "pubsub-helpers.h"
// #include "nfdc-helpers.h"
#include "repo-helpers.h"

using namespace ndn;
using namespace ndn::repo;
using namespace std;

class Options
{
public:
  Options()
    : home_prefix("/ndn-iot")
    , repo_name("/testrepo")
    , service(NDN_SD_LED)
    , identifier("/livingroom")
  {
  }
public:
  ndn::Name home_prefix;
  ndn::Name repo_name;
  uint8_t service;
  ndn::Name identifier;
};


class RepoInsert{
public:
  RepoInsert(const Name& home_prefix, 
             const Name& repo_name,
             const uint8_t service, 
             const Name& identifier)
    : m_home_prefix(home_prefix)
    , m_repo_name(repo_name)
    , m_service(service)
    , m_identifier(identifier)
  {
    m_scheduler = new Scheduler(m_face.getIoService());
    is_ready = false;
  }

  // TODO: remove face on SIGINT, SIGTERM

  void registerRoute(const Name& route_name, int face_id,
                     int cost = 0) 
  {
    Interest interest = prepareRibRegisterInterest(route_name, face_id, m_keyChain, cost);
    m_face.expressInterest(interest,
                           bind(&RepoInsert::onRegisterRouteDataReply, this, _1, _2),
                           bind(&RepoInsert::onNack, this, _1, _2),
                           bind(&RepoInsert::onTimeout, this, _1));
  }

  void sendSubInterest()
  {
    if (!is_ready)
      return;
    Name name(m_home_prefix);
    name.append(&m_service, sizeof(m_service));
    name.append("DATA");
    name.append(m_identifier);
    Interest interest(name);
    interest.setInterestLifetime(30_s);
    interest.setMustBeFresh(true);
    interest.setCanBePrefix(true);

    m_face.expressInterest(interest,
                           bind(&RepoInsert::onSubData, this, _1, _2),
                           bind(&RepoInsert::onNack, this, _1, _2),
                           bind(&RepoInsert::onTimeout, this, _1));
    cout << interest << std::endl;
  }


  void sendInsertInterest(const Name& to_insert)
  {
    if (!is_ready)
      return;

    //  /ucla/cs/repo/insert/<RepoCommandParameter>/<timestamp>/<random-value>/<SignatureInfo>/<SignatureValue>

    Name name(m_repo_name);
    name.append("insert");
    Buffer buffer = make_repo_insert_interest_parameter(to_insert);
    name.append(buffer.get<uint8_t>(), buffer.size());
    Interest interest(name);
    interest.setInterestLifetime(30_s);
    interest.setMustBeFresh(true);
    interest.setCanBePrefix(false);

    m_face.expressInterest(interest,
                           bind(&RepoInsert::onInsertData, this, _1, _2),
                           bind(&RepoInsert::onNack, this, _1, _2),
                           bind(&RepoInsert::onTimeout, this, _1));
    last_name = to_insert;
    cout << interest.getName() << std::endl;
  }

// private:
  void onInsertData(const Interest& interest, const Data& data)
  {
    std::cout << data << std::endl;

  }

  void onSubData(const Interest& interest, const Data& data)
  {
    std::cout << data << std::endl;
    cout << "begin insert" << endl;
    
    if (last_name == data.getName()) {
      cout << "same data, not insert" << endl;
      return;
    }
    sendInsertInterest(data.getName());
  }

  void onNack(const Interest& interest, const lp::Nack& nack)
  {
    std::cout << "received Nack with reason " << nack.getReason()
              << " for interest " << interest << std::endl;
  }

  void onTimeout(const Interest& interest)
  {
    std::cout << "Timeout " << interest << std::endl;
  }

  void onRegisterRouteDataReply(const Interest& interest, const Data& data)
  {
    Block response_block = data.getContent().blockFromValue();
    response_block.parse();

    Block status_code_block = response_block.get(STATUS_CODE);
    Block status_text_block = response_block.get(STATUS_TEXT);
    short response_code = readNonNegativeIntegerAs<int>(status_code_block);
    char response_text[1000] = {0};
    memcpy(response_text, status_text_block.value(), status_text_block.value_size());

    if (response_code == OK) {

      Block control_params = response_block.get(CONTROL_PARAMETERS);
      control_params.parse();

      Block name_block = control_params.get(ndn::tlv::Name);
      Name route_name(name_block);
      Block face_id_block = control_params.get(FACE_ID);
      int face_id = readNonNegativeIntegerAs<int>(face_id_block);
      Block origin_block = control_params.get(ORIGIN);
      int origin = readNonNegativeIntegerAs<int>(origin_block);
      Block route_cost_block = control_params.get(COST);
      int route_cost = readNonNegativeIntegerAs<int>(route_cost_block);
      Block flags_block = control_params.get(FLAGS);
      int flags = readNonNegativeIntegerAs<int>(flags_block);


      is_ready = true;
      std::cout << "RepoInsert: Bootstrap succeeded\n";

    }
    else {
      std::cout << "\nRegistration of route failed." << std::endl;
      std::cout << "Status text: " << response_text << std::endl;
    }
  }


public:
  bool is_ready = false;    // Ready after creating face and route to ND server
  Face m_face;
  KeyChain m_keyChain;
  Name m_home_prefix;
  Name m_repo_name;
  uint8_t m_service;
  Name m_identifier;
  Scheduler *m_scheduler;
  Name last_name;
};


class Program
{
public:
  explicit Program(const Options& options)
    : m_options(options)
  {
    // Init client
    m_client = new RepoInsert(m_options.home_prefix,
                              m_options.repo_name,
                              m_options.service, 
                              m_options.identifier);

    m_scheduler = new Scheduler(m_client->m_face.getIoService());
    // hard code face id 
    m_client->registerRoute(m_options.home_prefix, 404);
    m_client->registerRoute("/testrepo", 368);
    m_client->sendSubInterest();
    loop();
  }

  void loop() {
    m_client->sendSubInterest();
    m_scheduler->schedule(time::seconds(2), [this] {
      loop();
    });
  }

  ~Program() {
    delete m_client;
    delete m_scheduler;
  }

  RepoInsert *m_client;

private:
  const Options m_options;
  Scheduler *m_scheduler;
  boost::asio::io_service m_io_service;
};


int
main(int argc, char** argv)
{
  Options opt;
  Program program(opt);
  program.m_client->m_face.processEvents();
}