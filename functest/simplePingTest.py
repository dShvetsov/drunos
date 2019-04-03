#!/usr/bin/env python
import time

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

def test_twoports():
    topo = mininet.topo.SingleSwitchTopo(k=2, sopts={'protocols': "OpenFlow13"})
    net = Mininet(topo, controller=lambda name: RUNOS(name, profile='twoports'))
    net.start()
    time.sleep(0.5)
    loss = net.pingAll(timeout=1)
    net.stop()
    assert loss == 0


def test_twoinports():
    topo = mininet.topo.SingleSwitchTopo(k=2, sopts={'protocols': "OpenFlow13"})
    net = Mininet(topo, controller=lambda name: RUNOS(name, profile='twoinports'))
    net.start()
    time.sleep(0.5)
    loss = net.pingAll(timeout=1)
    net.stop()
    assert loss == 0


def test_dropall():
    topo = mininet.topo.SingleSwitchTopo(k=2, sopts={'protocols': "OpenFlow13"})
    net = Mininet(topo, controller=lambda name: RUNOS(name, profile='dropall'))
    net.start()
    time.sleep(0.5)
    loss = net.pingAll(timeout=1)
    net.stop()
    assert loss == 100


def test_two_switches():
    topo = mininet.topo.LinearTopo(k=2, n=1, sopts={'protocols': 'OpenFlow13'})
    net = Mininet(topo, controller=lambda name: RUNOS(name, profile='two_switch'))
    net.start()
    time.sleep(0.5)
    loss = net.pingAll(timeout=1)
    net.stop()
    assert loss == 0


def test_functions():
    topo = mininet.topo.SingleSwitchTopo(k=2, sopts={'protocols': 'OpenFlow13'})
    net = Mininet(topo, controller=lambda name: RUNOS(name, profile='func'))
    net.start()
    time.sleep(0.5)
    loss = net.pingAll()
    net.stop()
    assert loss == 0

def test_learning_switch():
    topo = mininet.topo.LinearTopo(k=4, n=1, sopts={'protocols': 'OpenFlow13'})
    net = Mininet(topo, controller=lambda name: RUNOS(name, profile='learning_switch'))
    net.start()
    time.sleep(0.5)
    loss = net.pingAll(timeout=1)
    net.stop()
    assert loss == 0



tests = [test_twoports, test_twoinports]

if __name__ == '__main__':
    test_twoports()
    test_twoinports()
    test_dropall()
    test_two_switches()
    test_functions()
    test_learning_switch()
