#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Sep 20 10:42:02 2019

@author: Macrobull
"""

from __future__ import absolute_import, division, unicode_literals

import logging, re

from multiprocessing import Process, Pipe

try:
    import tqdm_color_logging
except ImportError:
    pass


def kill_proc(proc:Process):
    """convenient function for some frontend + backend"""

    import os, signal

    os.kill(proc.pid, signal.SIGKILL)


class BasicRenderer(object):
    """

    implement your create_backend, create_frontend
    log_path: path to log file or SSH URL
    """

    def __init__(self, log_path:str,
                 *args, **kwargs):
        self.logger = logging.getLogger('BasicRenderer')
        self.log_path = log_path

        self.logger.debug('creating backend and frontend ...')
        self.backend, pipes = self.create_backend(log_path)
        self.frontend = self.create_frontend(pipes)

    def run(self):
        """run the backend service + frontend event loop"""

        self.logger.debug('starting backend ...')
        self.backend.start()

        self.logger.info('launching frontend ...')
        self.frontend.run()

        self.logger.info('exiting ...')
        self.backend.terminate()
        kill_proc(self.backend) # WORKAROUND: for some backends

    def create_backend(self, log_path:str)->'Tuple[Process, Sequence[Pipe]]':
        """
        default implementation to create a backend service,
        a backend service parses the log given by `log_path` asynchronously,
        send parsed data with data_pipe and receive command from control_pipe,
        the created service process and pipes is returned.
        see 'service' for more details
        """

        frontend_cpipe, backend_cpipe = Pipe(duplex=True)
        data_rpipe, data_spipe = Pipe(duplex=False)
        backend = Process(
                target=self.service,
                args=(backend_cpipe, data_spipe, log_path),
                )
        return backend, (frontend_cpipe, data_rpipe)

    def create_frontend(self, pipes:'Sequence[Pipe]')->'Any':
        """
        default implementation to create a frontend interface,
        the frontend instance should have the 'run()' method to start its event loop
        and interactivate with the backend service.
        'self.render' is called for the specific data interpolation
        see 'render' for more details
        """

        class Dummy(object):
            def __init__(self, pipes, renderer):
                self.renderer = renderer
                self.control_pipe, self.data_pipe = pipes

            def run(self):
                while True:
                    try:
                        self.renderer.render(*self.data_pipe.recv())
                    except BrokenPipeError:
                        self.renderer.logger.error('backend exited unexpectly')
                        break
                    except BaseException as e:
                        self.renderer.logger.error('frontend got exception:\n%s', e)
                        try:
                            self.control_pipe.send(True) # sample control command
                        except BrokenPipeError:
                            pass
                        break

        dummy_frontend = Dummy(pipes, self)
        return dummy_frontend

    def service(self, control_pipe:Pipe, data_pipe:Pipe, log_path:str):
        """the default backend service"""

        from ysl.backends import tailc, ssh_tailc
        from ysl.constructors import LogLoader
        from ysl.glog_parser import GlogParser, get_msg
        from ysl.parsers import frame_parser

        logger = logging.getLogger('service')
        logger.info('create default YSL parser')

        # auto local / remote tailc
        match = re.fullmatch(r'((\w+@)?.+):(.+)', log_path)
        if match is None:
            logger.info('with local file: %s', log_path)
            proc = tailc(log_path)
        else:
            address, _, log_path = match.groups()
            logger.info('with SSH remote file: %s', log_path)
            proc = ssh_tailc(address, log_path)

        glog_parser = GlogParser()
        record_stream = glog_parser.process(proc.stdout)
        msg_stream = get_msg(record_stream)
        frame_stream = frame_parser(msg_stream, yaml_loader_cls=LogLoader, persistent=True)

        for frame in frame_stream:
            if control_pipe.poll():
                command = control_pipe.recv()
                if command: # sample control command
                    logger.info('got exit command, exiting ...')
                    break

            data_pipe.send(frame)

    def render(self, frame:'Frame', document:'Any'):
        """the sample frontend: just print the frame and document"""

        self.logger.info('frame: %s', frame)
        self.logger.info('document: %s', document)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    renderer = BasicRenderer('/tmp/test.log')
    renderer.run()
