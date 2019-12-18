import numpy as np
import os
import numpy.random as rgn
from mpi4py import MPI

def analyse(path,nb_spike_detector):
    #Start communication channels
    path_to_files = path + nb_spike_detector + ".txt"
    #For NEST
    # Init connection
    print("Waiting for port details")
    info = MPI.INFO_NULL
    root=0
    port = MPI.Open_port(info)
    fport = open(path_to_files, "w+")
    fport.write(port)
    fport.close()
    print('wait connection '+port)
    comm = MPI.COMM_WORLD.Accept(port, info, root)
    print('connect to '+port)
    #test one rate
    status_ = MPI.Status()
    while(True):
        accept = np.array([True],dtype='b')
        comm.Send([accept,MPI.BOOL],dest=0,tag=0)
        data = np.empty(2, dtype='d')
        comm.Recv([data,2, MPI.DOUBLE],source=0,tag=MPI.ANY_TAG,status=status_)
        if status_.Get_tag() == 0:
            print(data)
            sys.stdout.flush()
        else:
            comm.Disconnect()
            comm = MPI.COMM_WORLD.Accept(port, info, root)

    MPI.Close_port(port)
    os.remove(path_to_files)
    print('exit');
    MPI.Finalize()

if __name__ == "__main__":
    import sys
    if len(sys.argv)==3:
        analyse(sys.argv[1],sys.argv[2])
    else:
        print('missing argument')

