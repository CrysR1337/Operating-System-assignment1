
import os
import argparse
import time


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--u", type=str, help="username")
    parser.add_argument("--p", type=str, help="password")
    parser.add_argument("--id",type=str, help="student ID")
    args=parser.parse_args()
    return args

def check_files(file_list):
    check_file_list=["dph","prod","cons","mycall"]
    for i in check_file_list:
        for j in file_list:
            if i not in file_list:
                return False
    return True


if __name__ == '__main__':
    args=get_args()
    git_info=os.popen("git clone https://{}:{}@github.com/sysu{}/os-assignment1.git"
                  .format(args.u,args.p,args.id)).readlines()
    time.sleep(10)
    os.system("cd ./os-assignment1/ && make all")
    time.sleep(5)
    file_list=os.listdir("./os-assignment1")
    if check_files(file_list):
        print("success")
    else:
        print("fail")













