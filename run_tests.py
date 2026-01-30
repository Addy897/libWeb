import os
import subprocess
TESTS_DIR = "./tests"
LIB_DIR = f"{TESTS_DIR}/libs"
def compile(filename:str) -> None:
    os.makedirs(LIB_DIR,exist_ok=True)
    subprocess.call([f"gcc -shared -o {LIB_DIR}/{filename}.a -fPIC {filename}.c"],shell=True)

def check_for(filename:str):
    test_path = f"{TESTS_DIR}/test_{filename}.py"
    if(os.path.exists(test_path)):
        compile(filename)
    else:
        print(f"[-] No Test for: {filename}.c")

if __name__ == '__main__':
    for file in os.listdir("."):
        if(file.endswith(".c")):
            check_for(file.removesuffix(".c"))
    subprocess.check_call([f"pytest && rm -rf f{LIB_DIR}"],shell=True)
    
                    

    
