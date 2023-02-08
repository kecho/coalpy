import sys
import argparse
import subprocess
import time
import json


def Wget(args, command, arg_data):
    return subprocess.run(['wget', '--post-data=%s' % arg_data, '-O', '-', "%s:%s/%s" % (args.server, args.port, command)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def WaitForCommand(args, post_args):
    result = Wget(args, "session_status", post_args)
    if result.returncode != 0:
        raise Exception("Failed at getting wait for commands. %s", resultstderr.decode('utf-8'))

    objstr = result.stdout.decode('utf-8')
    response_obj = json.loads(objstr)
    if response_obj["code"] != "Success":
        raise Exception("Failed at getting wait for commands. %s", response_obj["message"])

    status_obj = response_obj["message"]
    if status_obj["status"] != "STATE_RUNNING":
        success = status_obj["status"] == "STATE_COMPLETE_SUCCESS"
        return (True, success, status_obj["message"])
    else:
        return (False, True,  "")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="python -m coalpyci.agent",
        description="Wrapper of wget, sends post request to coalpyci server.")
    parser.add_argument('command')
    parser.add_argument('post_args')
    parser.add_argument('-s', '--server',
        default='http://localhost', required=False)
    parser.add_argument('-p', '--port',
        default='8080', required=False)
    parser.add_argument('-j', '--json',
        action="store_true", required=False)
    parser.add_argument('-w', '--wait',
        action="store_true", required=False)
    args = parser.parse_args()
    result = Wget(args, args.command, args.post_args) 
    if result.returncode == 0:
        response_obj = json.loads(result.stdout.decode('utf-8'))
        if args.json:
            print (str(response_obj))
            sys.exit(0)

        if response_obj["code"] == "Fail":
            print (response_obj["message"], file=sys.stderr)
            sys.exit(1)

        if args.command == "session_start":
            print (response_obj["message"]["session"])
        else:
            if args.wait:
                finished = False
                msg = ""
                success = False
                while not finished:
                    (finished, success, msg) = WaitForCommand(args, args.post_args)
                    time.sleep(1)
                print (msg)
                if success == False:
                    print ("Run ended in failure.", file=sys.stderr)
                    sys.exit(1)
            else:
                print (str(response_obj))
        sys.exit(0)
    else:
        print (result.stderr.decode('utf-8'), file=sys.stderr)
        sys.exit(0)
