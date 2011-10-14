// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

interface Mint factory MintImpl {

  Mint();

  Purse createPurse(int balance);

}


class MintImpl implements Mint {

  MintImpl() { }

  Purse createPurse(int balance) {
    return new PurseImpl(this, balance);
  }

}


interface Purse {

  int queryBalance();
  Purse sproutPurse();
  int deposit(int amount, Purse$Proxy source);

}


class PurseImpl implements Purse {

  PurseImpl(this._mint, this._balance) { }

  int queryBalance() {
    return _balance;
  }

  Purse sproutPurse() {
    return _mint.createPurse(0);
  }

  int deposit(int amount, Purse$Proxy purse) {
    Purse$ProxyImpl impl = purse.dynamic;  // TODO: Get rid of this 'cast'.
    PurseImpl source = impl.local;
    if (source._balance < amount) throw "Not enough dough.";
    _balance += amount;
    source._balance -= amount;
    //print("Moved $amount, leaving ${source._balance}");
    return _balance;
  }

  Mint _mint;
  int _balance;

}


class MintMakerPromiseTest {

  static void testMain() {
    Mint$Proxy mint = createMint();
    Purse$Proxy purse = mint.createPurse(100);
    expectEquals(100, purse.queryBalance());

    Purse$Proxy sprouted = purse.sproutPurse();
    expectEquals(0, sprouted.queryBalance());

    Promise<int> balance = sprouted.deposit(5, purse);
    expectEquals(0 + 5, balance);
    // FIXME(benl): because we have no ordering constraints we have to
    // manually order the messages or it all falls apart. We should
    // implement E-ORDER.
    balance.addCompleteHandler((_) {
      expectEquals(0 + 5, sprouted.queryBalance());
      expectEquals(100 - 5, purse.queryBalance());

      balance = sprouted.deposit(42, purse);
      expectEquals(0 + 5 + 42, balance);
      balance.addCompleteHandler((_) {
        expectEquals(0 + 5 + 42, sprouted.queryBalance());
        expectEquals(100 - 5 - 42, purse.queryBalance());
        // FIXME(benl): once more we could "pass" by not running anything much.
        expectDone(8);
      });
    });

  }

  static Mint$Proxy createMint() {
    Proxy isolate = new Proxy.forIsolate(new Mint$Dispatcher$Isolate());
    return new Mint$ProxyImpl(isolate);
  }


  static List<Promise> results;

  static void expectEquals(int expected, Promise<int> promise) {
    if (results === null) {
      results = new List<Promise>();
    }
    results.add(promise.then((int actual) {
      //print("done $expected/$actual");
      Expect.equals(expected, actual);
    }));
  }

  static void expectDone(int n) {
    if (results === null) {
      Expect.equals(0, n);
    } else {
      Promise done = new Promise();
      done.waitFor(results, results.length);
      done.then((ignored) {
        //print("expectDone $n/${results.length}");
        Expect.equals(n, results.length);
      });
    }
  }

}


// ---------------------------------------------------------------------------
// THE REST OF THIS FILE COULD BE AUTOGENERATED
// ---------------------------------------------------------------------------

interface Mint$Proxy {

  Purse$Proxy createPurse(int balance);  // Promise<int> balance.

}


class Mint$ProxyImpl extends Proxy implements Mint$Proxy {

  Mint$ProxyImpl(Proxy isolate) : super.forReply(isolate.call([null])) {}

  Purse$Proxy createPurse(int balance) {
    return new Purse$ProxyImpl(this.call([balance]));
  }

}


class Mint$Dispatcher extends Dispatcher<Mint> {

  Mint$Dispatcher(Mint mint) : super(mint) { }

  void process(var message, void reply(var response)) {
    int balance = message[0];
    Purse purse = target.createPurse(balance);
    SendPort port = Dispatcher.serve(new Purse$Dispatcher(purse));
    reply(port);
  }

}


class Mint$Dispatcher$Isolate extends Isolate {

  Mint$Dispatcher$Isolate() : super() { }

  void main() {
    this.port.receive((var message, SendPort replyTo) {
      Mint mint = new Mint();
      SendPort port = Dispatcher.serve(new Mint$Dispatcher(mint));
      Proxy proxy = new Proxy.forPort(replyTo);
      proxy.send([port]);
    });
  }

}


interface Purse$Proxy {

  Promise<int> queryBalance();
  Purse$Proxy sproutPurse();
  Promise<int> deposit(int amount, Purse$Proxy source);  // Promise<int> amount.

}


class Purse$ProxyImpl extends Proxy implements Purse$Proxy {

  Purse$ProxyImpl(Promise<SendPort> port) : super.forReply(port) { }

  Promise<int> queryBalance() {
    return this.call(["balance"]);
  }

  Promise<int> deposit(int amount, Purse$Proxy source) {
    return this.call(["deposit", amount, source]);
  }

  Purse$Proxy sproutPurse() {
    return new Purse$ProxyImpl(this.call(["sprout"]));
  }

}


class Purse$Dispatcher extends Dispatcher<Purse> {

  Purse$Dispatcher(Purse purse) : super(purse) { }

  void process(var message, void reply(var response)) {
    String command = message[0];
    //print("command $command");
    if (command == "balance") {
      int balance = target.queryBalance();
      reply(balance);
    } else if (command == "deposit") {
      int amount = message[1];
      Promise<SendPort> port =
        new PromiseProxy<SendPort>(new Promise<SendPort>.fromValue(message[2]));
      port.addCompleteHandler((_) {
        Purse$Proxy source = new Purse$ProxyImpl(port);
        int balance = target.deposit(amount, source);
        reply(balance);
      });
    } else if (command == "sprout") {
      Purse purse = target.sproutPurse();
      SendPort port = Dispatcher.serve(new Purse$Dispatcher(purse));
      reply(port);
    } else {
      // TODO: Send an exception back.
      reply("Exception: Command not understood");
    }
    //print("command $command done");
  }

}

main() {
  MintMakerPromiseTest.testMain();
}
