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
        time.sleep(15) # give controller time to start
        yield net
    finally:
        net.stop()


def run_example():
    topo = mininet.topo.LinearTopo(k=4, n=1, sopts={'protocols': 'OpenFlow13'})
    with run_mininet(topo, autoStaticArp=True, controller=profiled_runos('running_example')) as net:
        loss = net.pingAll(timeout=1)


if __name__ == '__main__':
    run_example()
