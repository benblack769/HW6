import subprocess
import time

cli_exec_name = "./client_time"

def poll_processes(opened_procs):
    procs_running = 1
    while procs_running:
        time.sleep(0.1)
        procs_running = len(opened_procs)
        for op_obj in opened_procs:
            if op_obj.poll():
                procs_running -= 1

def av_resp_time(num_clients):
    opened_procs = [subprocess.Popen([cli_exec_name,cli_n],universal_newlines=True)
                            for cli_n in range(num_clients)]

    poll_processes(opened_procs)

    tot_time = 0
    for proc in opened_procs:
        tot_time += float(proc.stdout.readlines()[0])

    return tot_time / num_clients


print(av_resp_time(5))
