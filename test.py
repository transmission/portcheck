import random
import requests
import socket
import subprocess
import time
import unittest

class PortcheckTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.client_port = random.randint(40000, 49999)
        cls.portcheck_port = random.randint(50000, 59999)
        cls.portcheck_process = subprocess.Popen(['./portcheck', str(cls.portcheck_port)])
        time.sleep(1)

    @classmethod
    def tearDownClass(cls):
        cls.portcheck_process.kill()
        cls.portcheck_process.communicate()

    def getPortcheckResult(self, ip):
        resp = requests.get(
          f'http://127.0.0.1:{self.portcheck_port}/{self.client_port}',
          headers = {'X-Forwarded-For': ip})
        self.assertEqual(resp.status_code, 200)
        return int(resp.content)

    def test_ipv4_open_only(self):
        s = socket.create_server(('127.0.0.1', self.client_port), family=socket.AF_INET, reuse_port=True)
        self.assertEqual(1, self.getPortcheckResult('127.0.0.1'))
        self.assertEqual(0, self.getPortcheckResult('::1'))
        s.close()

    def test_ipv6_open_only(self):
        s = socket.create_server(('::1', self.client_port), family=socket.AF_INET6, reuse_port=True)
        self.assertEqual(0, self.getPortcheckResult('127.0.0.1'))
        self.assertEqual(1, self.getPortcheckResult('::1'))
        s.close()
