#!/usr/bin/env python
import time
import requests
import simplejson
from contextlib import contextmanager

from mininet.net import Mininet
from mininet.util import dumpNodeConnections
from mininet.node import Controller
import mininet.topo


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
        time.sleep(0.5) # give controller time to start
        yield net
    finally:
        net.stop()

def test_twoports():
    topo = mininet.topo.SingleSwitchTopo(k=2, sopts={'protocols': "OpenFlow13"})
    with run_mininet(topo, controller=profiled_runos('twoports')) as net:
        loss = net.pingAll(timeout=1)
        assert loss == 0


def test_twoinports():
    topo = mininet.topo.SingleSwitchTopo(k=2, sopts={'protocols': "OpenFlow13"})
    with run_mininet(topo, controller=profiled_runos('twoinports')) as net:
        loss = net.pingAll(timeout=1)
        assert loss == 0


def test_dropall():
    topo = mininet.topo.SingleSwitchTopo(k=2, sopts={'protocols': "OpenFlow13"})
    with run_mininet(topo, controller=profiled_runos('dropall')) as net:
        loss = net.pingAll(timeout=1)
        assert loss == 100


def test_two_switches():
    topo = mininet.topo.LinearTopo(k=2, n=1, sopts={'protocols': 'OpenFlow13'})
    with run_mininet(topo, controller=profiled_runos('two_switch')) as net:
        loss = net.pingAll(timeout=1)
        assert loss == 0


def test_functions():
    topo = mininet.topo.SingleSwitchTopo(k=2, sopts={'protocols': 'OpenFlow13'})
    with run_mininet(topo, controller=profiled_runos('func')) as net:
        time.sleep(0.5)
        loss = net.pingAll()
        assert loss == 0


def test_topology():
    topo = mininet.topo.LinearTopo(k=2, n=1, sopts={'protocols': 'OpenFlow13'})
    with run_mininet (topo, controller=profiled_runos('learning_switch')) as net:
        time.sleep(0.5)
        response = requests.get('http://localhost:8000/api/topology/links')
        data = simplejson.loads(response.json())
        true_value_var1 = [
            {
                u'bandwidth': 5,
                u'ID': u'2383',
                u'connect': [
                    {u'src_port': 2, u'src_id': u'1'},
                    {u'dst_id': u'2', u'dst_port': 2}
                ]
            }
        ]
        true_value_var2 = [
            {
                u'bandwidth': 5,
                u'ID': u'2383',
                u'connect': [
                    {u'src_port': 2, u'src_id': u'2'},
                    {u'dst_id': u'1', u'dst_port': 2}
                ]
            }
        ]
        assert data == true_value_var1 or data == true_value_var2


def test_learning_switch():
    topo = mininet.topo.LinearTopo(k=4, n=1, sopts={'protocols': 'OpenFlow13'})
    with run_mininet(topo, controller=profiled_runos('learning_switch')) as net:
        loss = net.pingAll(timeout=1)
        assert loss == 0


tests = [test_twoports, test_twoinports]

if __name__ == '__main__':
#    test_twoports()
#    test_twoinports()
#    test_dropall()
#    test_two_switches()
#    test_functions()
    test_topology()
#    test_learning_switch()
