// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#library("test_runner");

#import("status_file_parser.dart");
#import("test_progress.dart");

/**
 * Classes and methods for executing tests.
 *
 * This module includes:
 * - Managing parallel execution of tests, including timeout checks.
 * - Evaluating the output of each test as pass/fail/crash/timeout.
 */

final int NO_TIMEOUT = 0;


String getBuildDir(Map configuration) {
  var buildDir = '';
  var os = configuration['os'];
  if (os == 'linux') {
    buildDir = 'out/';
  } else if (os == 'macos') {
    buildDir = 'xcodebuild/';
  }
  buildDir += (configuration['mode'] == 'debug') ? 'Debug_' : 'Release_';
  buildDir += configuration['architecture'] + '/';
  return buildDir;
}


String getExecutableName(Map configuration) {
  if (configuration['component'] == 'vm') {
    return 'dart_bin';
  } else if (configuration['component'] == 'dartc') {
    return 'dartc';
  } else {
    throw "Unknown executable for: ${configuration['component']}";
  }
}


String getDartShellFileName(Map configuration) {
  var name = getBuildDir(configuration) + getExecutableName(configuration);
  if (!(new File(name)).existsSync()) {
    throw "Executable '$name' does not exist";
  }
  return name;
}


class TestCase {
  String executablePath;
  List<String> arguments;
  String commandLine;
  String displayName;
  TestOutput output;
  Set<String> expectedOutcomes;
  Function completedHandler;

  TestCase(this.displayName, this.executablePath, this.arguments,
           this.completedHandler, this.expectedOutcomes) {
    commandLine = executablePath;
    for (var arg in arguments) {
      commandLine += " " + arg;
    }
  }

  bool get isNegative() => displayName.contains("NegativeTest");

  void completed() { completedHandler(this); }    
}


class TestOutput {
  // The TestCase this is the output from.
  TestCase testCase;
  int exitCode;
  bool timedOut;
  bool failed = false;
  List<String> stdout;
  List<String> stderr;
  Duration time;
  
  TestOutput(this.testCase, this.exitCode, this.timedOut, this.stdout,
             this.stderr, this.time) {
    testCase.output = this;
  }

  String get result() =>
      hasCrashed ? CRASH : (hasTimedOut ? TIMEOUT : (hasFailed ? FAIL : PASS));

  bool get unexpectedOutput() => !testCase.expectedOutcomes.contains(result);
  
  bool get hasCrashed() => !timedOut && exitCode != -1 && exitCode != 0;

  bool get hasTimedOut() => timedOut;

  bool get didFail() => exitCode != 0 && !hasCrashed;

  // Reverse result of a negative test.
  bool get hasFailed() => (testCase.isNegative ? !didFail : didFail);
}


class RunningProcess {
  Process process;
  TestCase testCase;
  int timeout;
  bool timedOut = false;
  Date startTime;
  Timer timeoutTimer;
  List<String> stdout;
  List<String> stderr;
  List<Function> handlers;

  RunningProcess(this.testCase, [this.timeout = NO_TIMEOUT]);

  void exitHandler(int exitCode) {
    new TestOutput(testCase, exitCode, timedOut, stdout,
                   stderr, new Date.now().difference(startTime));
    process.close();
    timeoutTimer.cancel();
    testCase.completed();
  }

  void makeReadHandler(StringInputStream source, List<String> destination) {
    return () {
      if (source.closed) return;  // TODO(whesse): Remove when bug is fixed.
      var line = source.readLine();
      while (null != line) {
        destination.add(line);
        line = source.readLine();
      }
    };
  }

  void start() {
    Expect.isFalse(testCase.expectedOutcomes.contains(SKIP));
    process = new Process(testCase.executablePath, testCase.arguments);
    process.exitHandler = exitHandler;
    startTime = new Date.now();
    process.start();
    
    InputStream stdoutStream = process.stdout;
    InputStream stderrStream = process.stderr;
    stdout = new List<String>();
    stderr = new List<String>();
    StringInputStream stdoutStringStream = new StringInputStream(stdoutStream);
    StringInputStream stderrStringStream = new StringInputStream(stderrStream);
    stdoutStringStream.dataHandler =
        makeReadHandler(stdoutStringStream, stdout);
    stderrStringStream.dataHandler =
        makeReadHandler(stderrStringStream, stderr);
    if (timeout != NO_TIMEOUT) {
      timeoutTimer = new Timer(timeoutHandler, 1000 * timeout, false);
    }
  }

  void timeoutHandler(Timer unusedTimer) {
    timedOut = true;
    process.kill();
  }
}


class ProcessQueue {
  int numProcesses = 0;
  final int maxProcesses;
  Queue<TestCase> tests;
  ProgressIndicator progress;

  ProcessQueue(this.maxProcesses, this.progress)
      : tests = new Queue<TestCase>();

  tryRunTest() {
    if (numProcesses < maxProcesses && !tests.isEmpty()) {
      TestCase test = tests.removeFirst();
      progress.start(test);
      // TODO(whesse): Refactor into various test output methods.
      Function old_callback = test.completedHandler;
      Function wrapper = (TestCase test_arg) {
        numProcesses--;
        progress.done(test_arg);
        tryRunTest();
        old_callback(test_arg);
      };
      test.completedHandler = wrapper;
        
      // TODO(whesse): Add timeout information to TestCase, use it here.
      new RunningProcess(test, 60).start();
      numProcesses++;
    }
  }

  runTest(TestCase test) {
    progress.testAdded();
    tests.add(test);
    tryRunTest();
  }
}
