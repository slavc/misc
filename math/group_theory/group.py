#!/usr/bin/python3

class Group:
    def __init__(self, G, op):
        self.G = G
        self.op = op
        self._cayley_table = None
        self._cyclic_subgroups = None

    def cayley_table(self):
        if not self._cayley_table:
            t = []
            for i in range(len(self.G) + 1):
                if i == 0:
                    t.append([0] + self.G)
                else:
                    t.append([self.G[i-1]] + self.G)
            for i in range(len(self.G)):
                for j in range(len(self.G)):
                    t[i+1][j+1] = self.op(t[i+1][0], t[0][j+1])
            self._cayley_table = t
        return self._cayley_table

    def cyclic_subgroups(self):
        if not self._cyclic_subgroups:
            m = {}
            for x in self.G:
                y = x
                H = [x]
                while True:
                    y = self.op(y, x)
                    if y == x:
                        H.insert(0, H.pop())
                        break
                    else:
                        H.append(y)
                m[x] = H
            self._cyclic_subgroups = m
        return self._cyclic_subgroups

def print_cayley_table(t):
    for i in range(len(t)):
        s = ''
        for j in range(len(t)):
            if i == 0 and j == 0:
                s += '     '
            else:
                s += '%4d ' % t[i][j]
        print(s)

def print_group_info(Gset, Gop):
    G = Group(Gset, eval(Gop))
    op_str = Gop.split(':')[-1]
    if len(Gset) < 30:
        print('Group: ({%s}, %s)' % (', '.join([str(x) for x in Gset]), op_str))
    else:
        print('Group: (<set ommitted due to length>, %s' % op_str)
    print('Cayley table:')
    print_cayley_table(G.cayley_table())
    print('Cyclic subgroups:')
    cgs = G.cyclic_subgroups()
    for x, cg in cgs.items():
        print('%s = {%s}' % (x, ', '.join([str(x) for x in cg])))

