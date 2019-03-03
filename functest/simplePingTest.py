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
    loss = net.pingAll()
    net.stop()
    assert loss == 0


def test_twoinports():
    topo = mininet.topo.SingleSwitchTopo(k=2, sopts={'protocols': "OpenFlow13"})
    net = Mininet(topo, controller=lambda name: RUNOS(name, profile='twoinports'))
    net.start()
    time.sleep(0.5)
    loss = net.pingAll()
    net.stop()
    assert loss == 0


def test_dropall():
    topo = mininet.topo.SingleSwitchTopo(k=2, sopts={'protocols': "OpenFlow13"})
    net = Mininet(topo, controller=lambda name: RUNOS(name, profile='dropall'))
    net.start()
    time.sleep(0.5)
    loss = net.pingAll(timeout=0.1)
    net.stop()
    assert loss == 100



tests = [test_twoports, test_twoinports]

if __name__ == '__main__':
    test_twoports()
    test_twoinports()
    test_dropall()
