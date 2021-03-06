
class IDBDatabase native "*IDBDatabase" {

  String name;

  String version;

  void addEventListener(String type, EventListener listener, [bool useCapture = null]) native;

  void close() native;

  IDBObjectStore createObjectStore(String name) native;

  void deleteObjectStore(String name) native;

  bool dispatchEvent(Event evt) native;

  void removeEventListener(String type, EventListener listener, [bool useCapture = null]) native;

  IDBVersionChangeRequest setVersion(String version) native;

  IDBTransaction transaction(String storeName, int mode) native;

  var dartObjectLocalStorage;

  String get typeName() native;
}
