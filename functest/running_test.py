#!/usr/bin/env python

import requests
import sys
import os
import time
import re
import simplejson as json
import threading
from contextlib import contextmanager
from collections import defaultdict

from mininet.net import Mininet
from mininet.util import dumpNodeConnections
from mininet.moduledeps import moduleDeps, pathCheck, TUN
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
        servout += server.monitor( timeoutms=1000 )
    server.sendInt()
    servout += server.waitOutput()
    debug( 'Server output: %s\n' % servout )
    result = [ self._parseIperf( servout ), self._parseIperf( cliout ) ]
    if l4Type == 'UDP':
        result.insert( 0, udpBw )
    output( '*** Results: %s\n' % result )
    return result

Mininet.timeout_iperf = timeout_iperf


class dRUNOS(Controller):
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


class RUNOS(Controller):
    """ original runos """
    def __init__(
            self,
            name,
            settings='network-settings-maple.json',
            port=6652,
            **kwargs
    ):
        """
        WATNING: Be aware, that settings file must starts with number of port!
        default: 6652_network-settings.json
        Yeah this is hack, but the easiest way to connect mininet script and RUNOS
        """
        Controller.__init__(
            self,
            name,
            port=port,
            command='orig_runos',
            cargs=settings,
            **kwargs
        )

    def start( self ):
        """
        Mininet forces to put port in cargs string. Hate it
        """
        pathCheck( self.command )
        cout = '/tmp/' + self.name + '.log'
        if self.cdir is not None:
            self.cmd( 'cd ' + self.cdir )
        self.cmd( self.command + ' ' + self.cargs +
                  ' 1>' + cout + ' 2>' + cout + ' &' )
        self.execed = False


def profiled_drunos(profile):
    return lambda name: dRUNOS(name, profile=profile)

def runos(name):
    return RUNOS(name)


@contextmanager
def run_mininet(*args, **kwargs):
    try:
        net = Mininet(*args, **kwargs)
        net.start()
        net.waitConnected(timeout=5.0, delay=1.0)
        time.sleep(1) # give controller time to start
        yield net
    finally:
        try:
            net.stop()
        except Exception as e:
            print('Unexpecting error: {}'.format(e))
            print('Call the clear mininet')
            os.system('mn -c')


class SmartSum(object):
    def __init__(self, first, second):
        self.trigged = False
        self.first = first
        self.second = second

    def __call__(self, text):
        def helper(line):
            if self.trigged and line.count(self.second) > 0:
                self.trigged = False
                return 1
            if line.count(self.first) > 0 :
                self.trigged = True
            else :
                self.trigged = False
            return 0

        ret =  reduce(lambda x,y : x + helper(y), text.split('\n'), 0)
        return ret


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


def run_snooping(net, messages):
    files = []
    for sw in net.switches:
        filename = "snoop_{}.dmp".format(sw.dpid)

        # running snoop in daemon (&) output in filename
        out_cmd = '> {} &'.format(filename)

        # Since snoop print in stderr, mode stderr to stoud, and stdout to deb null
        # Grep by needed strings
        sw.dpctl('snoop', '2>&1 >/dev/null | grep -E "{}" {}'.format("|".join(messages), out_cmd))
        files.append(filename)
    return files


def parse_snoop_files(files, messages):
    ret = defaultdict(lambda: 0)
    lldp_packet_ins = SmartSum("OFPT_PACKET_IN", "dl_type=0x88cc")
    lldp_packet_outs = SmartSum("OFPT_PACKET_OUT", "dl_type=0x88cc")
    for filename in files:
        with open(filename) as f:
            for line in f:
                for msg in messages:
                    ret[msg] += line.count(msg)
                ret['lldp_packet_ins'] += lldp_packet_ins(line)
                ret['lldp_packet_outs'] += lldp_packet_outs(line)
    return ret



def run_example(topo, prefix, controller):
    try:
        topo.dsh_name
    except AttributeError:
        print("Topo has no dsh_name")
        raise

    messages = [
        'OFPT_FLOW_MOD',
        'OFPT_PACKET_IN',
        'dl_type=0x88cc',
        'OFPT_PACKET_OUT',
        'OFPT_FLOW_REMOVED',
        'OFPT_GROUP_MOD',
    ]
    with run_mininet(topo, autoStaticArp=True, controller=controller) as net:
        print("start: {}: nodes {}, hosts {}".format(topo.dsh_name, len(topo.nodes()), len(topo.hosts())))
        files = run_snooping(net, messages)
        print("Snooping started")
        start_time = time.time()
        loss = net.pingAll(timeout=1)
        net.iperf(seconds=1, port=3000) # pass
        for port in range(2220, 2225):
            try:
                net.timeout_iperf(l4Type='UDP', port=port, seconds=1)
            except Exception as e:
                print(e)
        result = parse_snoop_files(files, messages)
        result['loss'] = loss
        result['hosts'] = len(topo.hosts())
        result['nodes'] = len(topo.nodes())
        result['topo'] = topo.dsh_name
        print(result)
        with open("{}_{}_{}.json".format(prefix, topo.dsh_name, len(topo.nodes())), 'a') as f:
            json.dump(result, f)



if __name__ == '__main__':
    topo = mininet.topo.LinearTopo(k=4, n=1, sopts={'protocols': 'OpenFlow13'})
    topo.dsh_name = 'Linear'
    if sys.argv[1] == 'drunos':
        run_example(topo, prefix='drunos_result', controller=profiled_drunos('running_example'))
    elif sys.argv[1] == 'maple':
        run_example(topo, prefix='maple_result', controller=runos)


