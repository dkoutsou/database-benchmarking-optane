import requests
import sys

sf=sys.argv[1]
config=sys.argv[2]
mysqlconfig=sys.argv[3]
users_list=[8, 16, 32, 64, 128]
matrix = []
VTUNE_SERVER_PATH=''
for users in users_list:
  row = []
  metrics = requests.get(VTUNE_SERVER_PATH+'-tpcc-sf{}/users{}-{}-{}/machine/sockets/0/pcies/metrics.json?'.format(sf,users,config,mysqlconfig))

  metrics = metrics.json()
  metrics_new = {}

  for metric in metrics["results"]:
    metrics_new[metric["name"]] = metric

  metric_list = ['Read Bandwidth',
                 'Write Bandwidth',
                 'Average Read Block Size',
                 'Average Write Block Size',
                 'Queue Depth', 'Average read latency (ms)',
                 'Average write latency (ms)']

  scaling_factor = [1024*1024, 1024*1024, 1024, 1024, 1, 1, 1]
  i = 0
  for metric in metric_list:
    sum = 0
    count = len(metrics_new[metric]["values"][6:-6])
    for value in metrics_new[metric]["values"][6:-6]:
      sum+=value[1]
    val = (sum/count)/scaling_factor[i]
    i = i + 1
    row.append(val)

############

  metrics = requests.get(VTUNE_SERVER_PATH+'-sf{}/users{}-{}-{}/machine/sockets/0/imcs/metrics.json?'.format(sf,users,config,mysqlconfig))

  metrics = metrics.json()
  metrics_new = {}

  for metric in metrics["results"]:
    metrics_new[metric["name"]] = metric

  metric_list = [ 'Read Hit Ratio', 'Write Hit Ratio' ]
  for metric in metric_list:
    sum = 0
    count = len( metrics_new[metric]["values"][6:-6])
    for value in metrics_new[metric]["values"][6:-6]:
      sum+=value[1]
    row.append(sum/count)

###########

  metrics = requests.get(VTUNE_SERVER_PATH+'-tpcc-sf{}/users{}-{}-{}/machine/sockets/0/metrics.json?'.format(sf,users,config,mysqlconfig))

  metrics = metrics.json()
  metrics_new = {}

  for metric in metrics["results"]:
    metrics_new[metric["name"]] = metric

  metric_list = [
          '3DXP Memory Bandwidth Read',
          '3DXP Memory Bandwidth Write',
          'Memory Read Bandwidth',
          'Memory Write Bandwidth' ]
  for metric in metric_list:
    sum = 0
    count = len(metrics_new[metric]["values"][3:-3])
    for value in metrics_new[metric]["values"][3:-3]:
      sum+=value[1]
    row.append(sum/count)

###########

  metrics = requests.get(VTUNE_SERVER_PATH + '-tpcc-sf{}/users{}-{}-{}/machine/metrics.json?'.format(sf,users,config,mysqlconfig))

  metrics = metrics.json()
  metrics_new = {}

  for metric in metrics["results"]:
    metrics_new[metric["name"]] = metric

  metric_list = ['CPU User', 'CPU System', 'CPU Wait' ]
  for metric in metric_list:
    sum = 0
    count = len(metrics_new[metric]["values"][6:-6])
    for value in metrics_new[metric]["values"][6:-6]:
      sum+=value[1]
    row.append(sum/count)
  matrix.append(row)

#print(matrix)
for i in range(len(matrix[0])):
  for j in range(len(matrix)):
    print("%.2f" % matrix[j][i], end=',')
  print()
