import random

class point():
    def __init__(self, x:int, y:int,z:int):
        self.x = x
        self.y = x
        self.z = x

    def __str__(self) :
        return "circuit.push_back(glm::vec3(" + str("%.2f" % self.x) + "f, " + str("%.2f" % self.y) + "f, " + str("%.2f" % self.z) + "f));"

def main():
    pstart = point(0, 0, 0)
    p = point(pstart.x, pstart.y, pstart.z)
    print(pstart)
    for i in range(10):
        l = 1
        p = point(p.x, p.y, p.z)

        v1 = random.random()
        p.x = p.x + v1
        l = l - v1

        v2 = random.random()
        p.z = p.z + v2
        l = l - v2

        p.y = p.y + l

        print(p)

if __name__ == "__main__":
    main()
