#  Copyright 2020 Forschungszentrum Jülich GmbH and Aix-Marseille Université
# "Licensed to the Apache Software Foundation (ASF) under one or more contributor license agreements; and to You under the Apache License, Version 2.0. "

from nest_elephant_tvb.Tvb.modify_tvb.test_interface import tvb_sim
import numpy as np

weight = np.array([[1,1,1,1],[1,1,1,1],[1,1,1,1],[1,1,1,1]])
delay = np.array([[1.5,1.5,1.5,1.5],[1.5,1.5,1.5,1.5],[1.5,1.5,1.5,1.5],[1.5,1.5,1.5,1.5]])
resolution_simulation = 0.1
resolution_monitor = 1.0
time_synchronize = 1.0
proxy_id =  [0,1]
firing_rate = np.array([[20.0,10.0]])*10**-3 # units time in tvb is ms so the rate is in KHz

sim = tvb_sim(weight, delay,proxy_id, resolution_simulation, resolution_monitor,time_synchronize)
time, result = sim(resolution_monitor,[np.array([resolution_simulation]),firing_rate])
for i in range(0,100):
    time,result = sim(time_synchronize,[time+resolution_monitor,np.repeat(firing_rate.reshape(1,2),int(resolution_monitor/resolution_simulation),axis=0)])
print('test succeeds')