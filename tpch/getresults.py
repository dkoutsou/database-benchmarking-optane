import requests

VTUNE_PATH=''
for i in range(1,23):
  metrics = requests.get(VTUNE_PATH)

  metrics = metrics.json()

  metrics_new = {}

  for metric in metrics["results"]:
    metrics_new[metric["name"]] = metric

  metric_list = [
          'CPU utilization %',
          'LLC Data Read Miss Latency(ns)',
          'Memory Read Bandwidth',
          'Memory Write Bandwidth',
          '3DXP Memory Bandwidth Read',
          '3DXP Memory Bandwidth Write',
          'NUMA % Reads addressed to local DRAM',
          'I/O Bandwidth Read',
          'I/O Bandwidth Write']


  for metric in metric_list:
    sum = 0
    count = len(metrics_new[metric]["values"])
    for value in metrics_new[metric]["values"]:
      sum+=value[1]

    print("%.2f" % (sum/count), end=',')
  print()
