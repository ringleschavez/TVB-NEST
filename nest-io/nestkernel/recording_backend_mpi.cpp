/*
 *  recording_backend_screen.cpp
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// C++ includes:
#include <iostream>


// Includes from nestkernel:
#include "recording_device.h"
#include "recording_backend_mpi.h"

void
nest::RecordingBackendMPI::initialize()
{
  auto nthreads = kernel().vp_manager.get_num_threads();
  device_map devices( nthreads );
  devices_.swap( devices );
  comm_map comms(nthreads);
  commMap_.swap(comms);
}

void
nest::RecordingBackendMPI::finalize()
{
  device_map::iterator it_device;
  for(it_device = devices_.begin();it_device!=devices_.end();it_device++){
    it_device->clear();
  }
  comm_map::iterator it_comm;
  for(it_comm = commMap_.begin();it_comm!=commMap_.end();it_comm++){
    it_comm->clear();
  }
  devices_.clear();
  commMap_.clear();
}

void
nest::RecordingBackendMPI::enroll( const RecordingDevice& device,
  const DictionaryDatum& params )
{
  if ( device.get_type() == RecordingDevice::SPIKE_DETECTOR )
  {
    thread tid = device.get_thread();
    index node_id = device.get_node_id();

    device_map::value_type::iterator device_it = devices_[ tid ].find( node_id );
    if ( device_it != devices_[ tid ].end() )
    {
      devices_[ tid ].erase( device_it );
    }

    std::pair< MPI_Comm*, const RecordingDevice*> pair = std::make_pair(nullptr,&device);
    devices_[ tid ].insert( std::make_pair( node_id,  pair) );
  }
  else
  {
    throw BadProperty( "Only spike detectors can record to recording backend 'mpi'." );
  }
}

void
nest::RecordingBackendMPI::disenroll( const RecordingDevice& device )
{
  const auto tid = device.get_thread();
  const auto node_id = device.get_node_id();

  auto device_it = devices_[ tid ].find( node_id );
  if ( device_it != devices_[ tid ].end() )
  {
    devices_[ tid ].erase( device_it );
  }
}

void
nest::RecordingBackendMPI::set_value_names( const RecordingDevice& device,
  const std::vector< Name >& double_value_names,
  const std::vector< Name >& long_value_names)
{
  // nothing to do
}

void
nest::RecordingBackendMPI::prepare()
{
  thread thread_id = kernel().vp_manager.get_thread_id();
  //get port and update the list of device
  device_map::value_type::iterator it_device;
  for(it_device=devices_[thread_id].begin();it_device != devices_[thread_id].end();it_device++){
    // add the link between MPI communicator and the device (device can share the same MPI communicator
	  char * port_name =new char [MPI_MAX_PORT_NAME];
	  get_port(it_device->second.second,port_name);
	  comm_map::value_type::iterator comm_it = commMap_[ thread_id ].find(port_name);
	  MPI_Comm * comm;
	  if (comm_it != commMap_[ thread_id ].end())
    {
	    comm = comm_it->second.first;
	    comm_it->second.second+=1;
	    delete[](port_name);
    } else {
      comm = new MPI_Comm;
      std::pair< MPI_Comm*, int> comm_count = std::make_pair(comm,1);
      commMap_[thread_id].insert(std::make_pair(port_name, comm_count));
	  }
	  it_device->second.first=comm;
  }

  // connect the thread with MPI process it need to be connected
  // WARNING can be a bug if it's need all the thread to be connected in MPI
  comm_map::value_type::iterator it_comm;
  for ( it_comm = commMap_[thread_id].begin(); it_comm != commMap_[thread_id].end(); it_comm++){
    printf("Connect to %s\n", it_comm->first);fflush(stdout);
    MPI_Comm_connect(it_comm->first, MPI_INFO_NULL, 0, MPI_COMM_WORLD, it_comm->second.first); // should use the status for handle error
  }
}

void
nest::RecordingBackendMPI::pre_run_hook()
{
  // Waiting until all the receptor are ready to receive information
  const thread thread_id = kernel().vp_manager.get_thread_id();
  device_map::value_type::iterator it_device;
  for (it_device = devices_[thread_id].begin(); it_device != devices_[thread_id].end(); it_device++) {
    bool accept_starting[1];
    MPI_Status status_mpi;
    MPI_Recv(accept_starting, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG , *it_device->second.first ,&status_mpi);
  }
  // future improvement is to save the source of the signal (meaning can be different MPI sources to send also)
}


void
nest::RecordingBackendMPI::post_step_hook()
{
  // nothing to do
}

void
nest::RecordingBackendMPI::post_run_hook()
{
  thread thread_id = kernel().vp_manager.get_thread_id();
  // WARNING can be a bug if all the thread to send ending connection of MPI
  comm_map::value_type::iterator it_comm;
  for (it_comm = commMap_[thread_id].begin(); it_comm != commMap_[thread_id].end(); it_comm++) {
    int value[1];
    value[0] = thread_id;
    MPI_Send(value, 1, MPI_INT, 0, 1, *it_comm->second.first);
  }
}

void
nest::RecordingBackendMPI::cleanup()
{
  thread thread_id = kernel().vp_manager.get_thread_id();
  // WARNING can be a bug if all the thread to send ending connection of MPI
  //disconnect MPI
  comm_map::value_type::iterator it_comm;
  for (it_comm = commMap_[thread_id].begin(); it_comm != commMap_[thread_id].end(); it_comm++) {
    int value[1];
    value[0] = thread_id;
    MPI_Send(value, 1, MPI_INT, 0, 1, *it_comm->second.first);
    MPI_Comm_disconnect(it_comm->second.first);
    delete(it_comm->second.first);
    delete[](it_comm->first);
  }
  // clear map of device
  commMap_[thread_id].clear();
  device_map::value_type::iterator it_device;
  for (it_device = devices_[thread_id].begin(); it_device != devices_[thread_id].end(); it_device++) {
    it_device->second.first= nullptr;
  }
}

void
nest::RecordingBackendMPI::check_device_status( const DictionaryDatum& params ) const
{
  // nothing to do
}

void
nest::RecordingBackendMPI::get_device_defaults( DictionaryDatum& params ) const
{
  // nothing to do
}

void
nest::RecordingBackendMPI::get_device_status( const nest::RecordingDevice& device,
  DictionaryDatum& params_dictionary ) const
{
  // nothing to do
}



void
nest::RecordingBackendMPI::write( const RecordingDevice& device,
  const Event& event,
  const std::vector< double >&,
  const std::vector< long >& )
{
  const thread thread_id = kernel().get_kernel_manager().vp_manager.get_thread_id();
  const index sender = event.get_sender_node_id();
  const Time stamp = event.get_stamp();

  MPI_Comm* comm;
  device_map::value_type::iterator it_devices = devices_[thread_id].find(device.get_node_id());
  if (it_devices != devices_[thread_id].end()) {
    comm = it_devices->second.first;
  } else {
    throw BackendPrepared( " Internal error " );
  }
  double passed_num[2]= {double(sender), stamp.get_ms()};
  MPI_Send(&passed_num, 2, MPI_DOUBLE, 0, thread_id, *comm);
}

/* ----------------------------------------------------------------
 * Parameter extraction and manipulation functions
 * ---------------------------------------------------------------- */
void
nest::RecordingBackendMPI::get_status( DictionaryDatum& d ) const
{
  //nothing to do
}

void
nest::RecordingBackendMPI::set_status( const DictionaryDatum& d )
{
  //nothing to do
}

void
nest::RecordingBackendMPI::get_port(const RecordingDevice* device, char* port_name) {
  get_port(device->get_node_id(),device->get_label(),port_name);
}

void
nest::RecordingBackendMPI::get_port(const index index_node, const std::string& label, char* port_name){
  std::ostringstream basename;
  const std::string& path = kernel().io_manager.get_data_path();
  if ( not path.empty() )
  {
    basename << path << '/';
  }
  basename << kernel().io_manager.get_data_prefix();

  if ( not label.empty() )
  {
    basename << label;
  }
  else {
     //TODO take in count this case
  }
  char add_path[150];
  sprintf(add_path, "/%zu.txt", index_node);
  basename << add_path;
  std::cout << basename.rdbuf() << std::endl;
  std::ifstream file(basename.str());
  if (file.is_open()) {
    file.getline(port_name, 256);
  }
  file.close();
}
