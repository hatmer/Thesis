import simpy

def app(env):
    while True:
        print("doing app stuff")
        yield env.timeout(3)







env = simpy.Environment()
env.process(app(env))
env.run(until=15)

