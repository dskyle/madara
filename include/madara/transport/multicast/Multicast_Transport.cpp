#include "madara/transport/multicast/Multicast_Transport.h"
#include "madara/transport/multicast/Multicast_Transport_Read_Thread.h"
#include "madara/transport/Transport_Context.h"
#include "madara/utility/Log_Macros.h"
#include "madara/transport/Reduced_Message_Header.h"
#include "madara/utility/Utility.h"
#include "madara/expression_tree/Expression_Tree.h"
#include "madara/expression_tree/Interpreter.h"
#include "madara/transport/Fragmentation.h"

#include <iostream>

Madara::Transport::Multicast_Transport::Multicast_Transport (const std::string & id,
        Madara::Knowledge_Engine::Thread_Safe_Context & context, 
        Settings & config, bool launch_transport)
: Base (id, config, context),
  valid_setup_ (false),
  write_socket_ (ACE_sap_any_cast (ACE_INET_Addr &), PF_INET, 0, 1)
{
  // create a reference to the knowledge base for threading
  knowledge_.use (context);

  // set the data plane for the read threads
  read_threads_.set_data_plane (knowledge_);

  if (launch_transport)
    setup ();
}

Madara::Transport::Multicast_Transport::~Multicast_Transport ()
{
  close ();
}

void
Madara::Transport::Multicast_Transport::close (void)
{
  this->invalidate_transport ();

  read_threads_.terminate ();

  read_threads_.wait ();
  
  write_socket_.close ();
}

int
Madara::Transport::Multicast_Transport::reliability (void) const
{
  return Madara::Transport::BEST_EFFORT;
}

int
Madara::Transport::Multicast_Transport::reliability (const int &)
{
  return Madara::Transport::BEST_EFFORT;
}

int
Madara::Transport::Multicast_Transport::setup (void)
{
  // call base setup method to initialize certain common variables
  Base::setup ();

  // resize addresses to be the size of the list of hosts
  addresses_.resize (this->settings_.hosts.size ());

  int ttl = 1;
  int send_buff_size = 0, tar_buff_size (settings_.queue_length);
  int rcv_buff_size = 0;
  int opt_len = sizeof (int);

  write_socket_.set_option (IPPROTO_IP,
                     IP_MULTICAST_TTL,
                     (void *) &ttl,
                     sizeof (ttl));

  write_socket_.get_option (SOL_SOCKET, SO_SNDBUF,
    (void *)&send_buff_size, &opt_len);

  write_socket_.get_option (SOL_SOCKET, SO_RCVBUF,
    (void *)&rcv_buff_size, &opt_len);
  
  MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
    DLINFO "Multicast_Transport::setup:" \
    " default socket buff size is send=%d, rcv=%d\n",
    send_buff_size, rcv_buff_size));
  
  if (send_buff_size < tar_buff_size)
  {
    MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
      DLINFO "Multicast_Transport::setup:" \
      " setting send buff size to settings.queue_length (%d)\n",
      tar_buff_size));
  
    write_socket_.set_option (SOL_SOCKET, SO_SNDBUF,
      (void *) &tar_buff_size, opt_len);
    
    write_socket_.get_option (SOL_SOCKET, SO_SNDBUF,
      (void *)&send_buff_size, &opt_len);
    
    MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
      DLINFO "Multicast_Transport::setup:" \
      " current socket buff size is send=%d, rcv=%d\n",
      send_buff_size, rcv_buff_size));
  }
  
  if (rcv_buff_size < tar_buff_size)
  {
    MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
      DLINFO
      "Multicast_Transport::setup:" \
      " setting rcv buff size to settings.queue_length (%d)\n",
      tar_buff_size));
  
    write_socket_.set_option (SOL_SOCKET, SO_SNDBUF,
      (void *) &tar_buff_size, opt_len);
    
    write_socket_.get_option (SOL_SOCKET, SO_SNDBUF,
      (void *)&rcv_buff_size, &opt_len);
    
    MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
      DLINFO
      "Multicast_Transport::setup:" \
      " current socket buff size is send=%d, rcv=%d\n",
      send_buff_size, rcv_buff_size));
  }
    
  if (addresses_.size () > 0)
  {
    // convert the string host:port into an ACE address
    for (unsigned int i = 0; i < addresses_.size (); ++i)
    {
      addresses_[i].set (settings_.hosts[i].c_str ());

      MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
        DLINFO "Multicast_Transport::setup:" \
        " settings address[%d] to %s:%d\n", i, 
        addresses_[i].get_host_addr (), addresses_[i].get_port_number ()));
    }
    
    int port = addresses_[0].get_port_number ();
    const char * host = addresses_[0].get_host_addr ();

    if (-1 == read_socket_.join (addresses_[0], 1))
    {
      MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
        DLINFO "Multicast_Transport::setup:" \
        " Error subscribing to multicast address %s:%d\n", host, port));
    } 
    else
    {
      MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
        DLINFO "Multicast_Transport::setup:" \
        " Success subscribing to multicast address %s:%d\n", host, port));
    
      int send_buff_size = 0, tar_buff_size (settings_.queue_length);
      int rcv_buff_size = 0;
      int opt_len = sizeof (int);
    
      ACE_SOCK_Dgram & bare_socket = read_socket_;

      bare_socket.get_option (SOL_SOCKET, SO_RCVBUF,
        (void *)&rcv_buff_size, &opt_len);
  
      MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
        DLINFO
        "Multicast_Transport::setup:" \
        " default socket buff size is send=%d, rcv=%d\n",
        send_buff_size, rcv_buff_size));
  
      if (send_buff_size < tar_buff_size)
      {
        MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
          DLINFO
          "Multicast_Transport::setup:" \
          " setting send buff size to settings.queue_length (%d)\n",
          tar_buff_size));
  
        bare_socket.set_option (SOL_SOCKET, SO_SNDBUF,
          (void *)&tar_buff_size, opt_len);
    
        bare_socket.get_option (SOL_SOCKET, SO_SNDBUF,
          (void *)&send_buff_size, &opt_len);
    
        MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
          DLINFO
          "Multicast_Transport::setup:" \
          " current socket buff size is send=%d, rcv=%d\n",
          send_buff_size, rcv_buff_size));
      }
  
      if (rcv_buff_size < tar_buff_size)
      {
        MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
          DLINFO
          "Multicast_Transport::setup:" \
          " setting rcv buff size to settings.queue_length (%d)\n",
          tar_buff_size));
  
        bare_socket.set_option (SOL_SOCKET, SO_SNDBUF,
          (void *)&tar_buff_size, opt_len);
    
        bare_socket.get_option (SOL_SOCKET, SO_SNDBUF,
          (void *)&rcv_buff_size, &opt_len);
    
        MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
          DLINFO
          "Multicast_Transport::setup:" \
          " current socket buff size is send=%d, rcv=%d\n",
          send_buff_size, rcv_buff_size));
      }
    }

    double hertz = settings_.read_thread_hertz;
    if (hertz < 0.0)
    {
      hertz = 0.0;
    }
    
    MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
      DLINFO "Multicast_Transport::setup:" \
      " starting %d threads at %f hertz\n", settings_.read_threads, 
      hertz));

    for (uint32_t i = 0; i < settings_.read_threads; ++i)
    {
      std::stringstream thread_name;
      thread_name << "read";
      thread_name << i;

      read_threads_.run (hertz, thread_name.str (),
        new Multicast_Transport_Read_Thread (
          settings_, id_, addresses_[0], write_socket_, read_socket_,
          send_monitor_, receive_monitor_, packet_scheduler_));
    }

    // start thread with the addresses (only looks at the first one for now)
    //thread_ = new Madara::Transport::Multicast_Transport (
    //                settings_, id_, context_, addresses_[0], write_socket_,
    //                send_monitor_, receive_monitor_, packet_scheduler_);
  }

  

  return this->validate_transport ();
}

long
Madara::Transport::Multicast_Transport::send_data (
  const Madara::Knowledge_Records & orig_updates)
{
  const char * print_prefix = "Multicast_Transport::send_data";

  long result = prep_send (orig_updates, print_prefix);

  if (addresses_.size () > 0 && result > 0)
  {
    uint64_t bytes_sent = 0;
    uint64_t packet_size = Message_Header::get_size (buffer_.get_ptr ());

    if (packet_size > settings_.max_fragment_size)
    {
      Fragment_Map map;
      
      MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
        DLINFO "%s:" \
        " fragmenting %Q byte packet (%d bytes is max fragment size)\n",
        print_prefix, packet_size, settings_.max_fragment_size));

      // fragment the message
      frag (buffer_.get_ptr (), settings_.max_fragment_size, map);

      for (Fragment_Map::iterator i = map.begin (); i != map.end (); ++i)
      {
        // send the fragment
        bytes_sent += write_socket_.send(
          i->second,
          (ssize_t)Message_Header::get_size (i->second),
          addresses_[0]);

        // sleep between fragments, if such a slack time is specified
        if (settings_.slack_time > 0)
          Madara::Utility::sleep (settings_.slack_time);
      }
      
      send_monitor_.add ((uint32_t)bytes_sent);

      MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
        DLINFO "%s:" \
        " Sent fragments totalling %Q bytes\n",
        print_prefix, bytes_sent));

      delete_fragments (map);
    }
    else
    {
      bytes_sent = write_socket_.send(
        buffer_.get_ptr (), (ssize_t)result, addresses_[0]);
      MADARA_DEBUG (MADARA_LOG_MAJOR_EVENT, (LM_DEBUG, 
        DLINFO "%s:" \
        " Sent packet of size %Q\n",
        print_prefix, bytes_sent));
      send_monitor_.add ((uint32_t)bytes_sent);
    }


    MADARA_DEBUG (MADARA_LOG_MINOR_EVENT, (LM_DEBUG, 
      DLINFO "%s:" \
      " Send bandwidth = %d B/s\n",
      print_prefix, send_monitor_.get_bytes_per_second ()));

    result = (long) bytes_sent;
  }
  
  return result;
}