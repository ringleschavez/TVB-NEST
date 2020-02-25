from nest_elephant_tvb.simulation.file_tvb.test_interface_co_parallel.function_tvb import tvb_sim
import numpy as np
import numpy.random as rgn

weight = np.array([[2,8,0],[0,0,0],[3,0,1]])
delay = np.array([[0.6,0.5,1.0],[0.7,0.8,3.0],[1.0,0.5,0.7]])
init_value = np.array([[0.1,0.1,0.2]]*2)
initial_condition = init_value.reshape(2,1,weight.shape[0],1)
resolution_simulation = 0.1
resolution_monitor = 0.1*4
time_synchronize = np.min(delay)
proxy_id = [0]
not_proxy_id = np.where(np.logical_not(np.isin(np.arange(0,weight.shape[0],1), proxy_id)))[0]

# full simulation
rgn.seed(42)
sim_ref = tvb_sim(weight, delay,[], resolution_simulation, resolution_monitor,time_synchronize,initial_condition=initial_condition)
time,result_ref,s_ref = sim_ref(resolution_monitor,s=True)
delai_input = [time,result_ref[:,proxy_id][:,:,0]]

# simulation with one proxy
rgn.seed(42)
sim = tvb_sim(weight, delay,proxy_id, resolution_simulation, resolution_monitor,time_synchronize,initial_condition=initial_condition)
time,result,s = sim(resolution_monitor,None,s=True)

diff = np.where(np.squeeze(result_ref[:,not_proxy_id],axis=2)[0] != np.squeeze(result,axis=2)[0])
if diff[0].size != 0:
    print("Fail H compare")
    print(diff)
    print(result_ref)
    print(result)
else:
    print('test 0 : succeed')
diff_s = np.where(np.squeeze(s_ref,axis=2)[0] != np.squeeze(s,axis=2)[0])
if diff_s[0].size != 0:
    print("Fail S compare")
    print(diff_s)
    print(s_ref)
    print(s)
else:
    print('test 0 : succeed')

max_error_h = 0.0
max_error_s = 0.0
for i in range(0,10000):
    time,result_ref,s_ref = sim_ref(time_synchronize,s=True)
    time, result, s = sim(time_synchronize,delai_input, s=True)
    delai_input = [time, result_ref[:, proxy_id][:, :, 0]]

    diff = np.where(result_ref[:,not_proxy_id]!= result)
    if np.max(np.abs(result_ref[:,not_proxy_id]- result)) > max_error_h:
        max_error_h = np.max(np.abs(result_ref- result))
    if diff[0].size !=0:
        print('rate compare')
        print(diff)
        print(result_ref)
        print(np.max(np.abs(result_ref- result)))
    else:
        print('test H '+str(i+1)+' : succeed')
    diff_s = np.where(s_ref != s)
    if np.max(np.abs(s_ref - s)) > max_error_s:
        max_error_s = np.max(np.abs(s_ref - s))
    if diff_s[0].size != 0:
        print("s_compare")
        print(diff_s)
        print(s_ref)
        print(s)
        print(np.max(np.abs(s_ref - s)))
    else:
        print('test S ' + str(i + 1) + ' : succeed')

print("time_synchronize : %r s"%(time_synchronize*10**-3))
print("time_monitor : %r s"%(resolution_monitor*10**-3))
print("max error firing rate : %r Hz" % (max_error_h*10**3))
print("max error S rate : %r" % max_error_s)

