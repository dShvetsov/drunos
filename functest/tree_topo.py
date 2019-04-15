from mininet.topo import Topo

class Node(object):
    def __init__(self):
        self.left = None
        self.right = None

    def addChild(self, child):
        if self.left is None:
            self.left = child
            return
        elif self.right is None:
            self.right = child
            return

    def isHost(self):
        return self.left is None and self.right is None

def createTopo(n):
    node = Node()
    if n > 1:
        node.addChild(createTopo(n/2))
        node.addChild(createTopo(n - n/2))
    return node


class BinTreeTopo(Topo):

    def build_tree(self, node):
        if node.isHost():
            self.hostName += 1
            return self.addHost("h%s" % self.hostName)
        else:
            self.switchName += 1
            sw = self.addSwitch("s%s" % self.switchName, **self.sopts)
            self.sws.append(sw)
            left_subtree = self.build_tree(node.left)
            right_subtree = self.build_tree(node.right)
            self.addLink(sw, left_subtree)
            self.addLink(sw, right_subtree)
            return sw

    def __init__(self, n=2, sopts=None):
        self.sws = []
        self.switchName = 0
        self.hostName= 0
        Topo.__init__( self )
        root = createTopo( n )
        self.build_tree( root )
        self.sopts = sopts

