import os
commandCompile = 'g++ NanoDesignTask.cpp'
commandRun1 = './a.out amd64'
commandRun2 = './a.out armv7e'
os.system(commandCompile)

#amd64
#os.system(commandRun1)

#armv7e
os.system(commandRun2)

print("OK/CHYBA")