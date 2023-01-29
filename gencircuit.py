import math
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
        theta = math.pi/2 * random.random()
        phi = math.acos(random.random()*2 - 1)

        p = point(p.x, p.y, p.z)

        p.x = p.x + math.cos(theta) * math.sin(phi)
        p.y = p.y + math.sin(theta) * math.sin(phi)
        p.z = p.z + math.cos(phi)

        print(p)

if __name__ == "__main__":
    main()
