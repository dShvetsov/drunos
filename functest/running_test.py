import requests
import time
import re
import simplejson
import threading
from contextlib import contextmanager

from mininet.net import Mininet
from mininet.util import dumpNodeConnections
from mininet.node import Controller
import mininet.topo


from mininet.log import info, error, debug, output, warn
def timeout_iperf( self, hosts=None, l4Type='TCP', udpBw='10M', fmt=None,
           seconds=5, port=5001, timeout=5):
    """Run iperf between two hosts.
       hosts: list of hosts; if None, uses first and last hosts
       l4Type: string, one of [ TCP, UDP ]
       udpBw: bandwidth target for UDP test
       fmt: iperf format argument if any
       seconds: iperf time to transmit
       port: iperf port
       returns: two-element array of [ server, client ] speeds
       note: send() is buffered, so client rate can be much higher than
       the actual transmission rate; on an unloaded system, server
       rate should be much closer to the actual receive rate"""
    hosts = hosts or [ self.hosts[ 0 ], self.hosts[ -1 ] ]
    assert len( hosts ) == 2
    client, server = hosts
    output( '*** Iperf: testing', l4Type, 'bandwidth between',
            client, 'and', server, '\n' )
    server.cmd( 'killall -9 iperf' )
    iperfArgs = 'timeout %d iperf -p %d ' % (timeout, port)
    bwArgs = ''
    if l4Type == 'UDP':
        iperfArgs += '-u '
        bwArgs = '-b ' + udpBw + ' '
    elif l4Type != 'TCP':
        raise Exception( 'Unexpected l4 type: %s' % l4Type )
    if fmt:
        iperfArgs += '-f %s ' % fmt
    start_time = time.time()
    server.sendCmd( iperfArgs + '-s' )
    if l4Type == 'TCP':
        if not waitListening( client, server.IP(), port ):
            raise Exception( 'Could not connect to iperf on port %d'
                             % port )
    cliout = client.cmd( iperfArgs + '-t %d -c ' % seconds +
                         server.IP() + ' ' + bwArgs )
    debug( 'Client output: %s\n' % cliout )
    servout = ''
    # We want the last *b/sec from the iperf server output
    # for TCP, there are two of them because of waitListening
    count = 2 if l4Type == 'TCP' else 1
    while len( re.findall( '/sec', servout ) ) < count:
        if time.time() - start_time > timeout:
            break
        servout += server.monitor( timeoutms=5000 )
    server.sendInt()
    servout += server.waitOutput()
    debug( 'Server output: %s\n' % servout )
    result = [ self._parseIperf( servout ), self._parseIperf( cliout ) ]
    if l4Type == 'UDP':
        result.insert( 0, udpBw )
    output( '*** Results: %s\n' % result )
    return result

Mininet.timeout_iperf = timeout_iperf


class RUNOS(Controller):
    def __init__(
            self,
            name,
            settings='network-settings.json',
            profile='default',
            port=6652,
            **kwargs
    ):
        Controller.__init__(
            self,
            name,
            port=port,
            command='runos',
            cargs='--config %s --profile %s --unusedport %s' % (settings, profile, "%s"),
            **kwargs
        )


def profiled_runos(profile):
    return lambda name: RUNOS(name, profile=profile)


@contextmanager
def run_mininet(*args, **kwargs):
    try:
        net = Mininet(*args, **kwargs)
        net.start()
        net.waitConnected(timeout=5.0, delay=1.0)
        time.sleep(1) # give controller time to start
        yield net
    finally:
        net.stop()


def deadlined_iperf(net, port, timeout=5):
    server = net.hosts[0]
    client = net.hosts[-1]
    iperf = lambda: net.iperf(seconds=1, port=port)
    t = threading.Thread(target=iperf)
    t.start()
    t.join(3)
    server.cmd('killall -9 iperf')
    client.cmd('killall -9 iperf')
    t.join()


def run_example():
    topo = mininet.topo.LinearTopo(k=4, n=1, sopts={'protocols': 'OpenFlow13'})
    with run_mininet(topo, autoStaticArp=True, controller=profiled_runos('running_example')) as net:
        # loss = net.pingAll(timeout=1)
        # net.iperf(seconds=1, port=3000) # pass
        for port in range(2220, 2225):
            try:
                net.timeout_iperf(l4Type='UDP', port=port, seconds=1)
            except Exception as e:
                print(e)



if __name__ == '__main__':
    run_example()
