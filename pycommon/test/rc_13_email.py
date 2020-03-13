import unittest
import os
import json
import mailbox

from reporter_config.Config import Config

class RCEMailTest(unittest.TestCase):

    def setUp(self):
        """
        Example message created by a conv function in a reporter
        """
        with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
            self.msg = json.load(f)


    def tearDown(self):
        pass

    @unittest.skip("skipping send email test 1")
    def test_01_send_email(self):
        """
        Load email.yaml configuration file, parse it and analyze it

        This shouldn't rise any exceptions
        """
        self.config = Config(os.path.dirname(__file__) + '/rc_config/email.yaml');

        self.assertNotEqual(self.config, None)
        self.config.match(self.msg)

        # Check if mail arrived in /var/mail/nemea
        # TODO: parametrize the user
        m = mailbox.mbox('/var/mail/nemea')

        for key in m.iterkeys():
            # To be sure it is our email we will match TO, FROM and SUBJECT fields
            if (m[key]['subject'] == 'Attempt.Login (cz.uhk.apate.cowrie): 1.2.3.4 -> 195.113.165.128/25' and
                m[key]['from'] == 'nemea@localhost.localdomain' and
                m[key]['to'] == 'nemea@localhost.localdomain'):
                    # Remove given email from mailbox
                    # There is some wierd behaviour regarding un/locking and flushing
                    # Only worked for me when tests were run as root
                    m.lock()
                    m.discard(key)
                    m.unlock()
                    m.flush()
                    m.close()
                    return

        self.fail("No email was received")

