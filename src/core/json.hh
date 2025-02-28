#ifndef SSC_SOCKET_JSON_HH
#define SSC_SOCKET_JSON_HH

#include "../common.hh"

namespace SSC::JSON {
  // forward
  class Any;
  class Raw;
  class Null;
  class Object;
  class Array;
  class Boolean;
  class Number;
  class String;

  using ObjectEntries = std::map<std::string, Any>;
  using ArrayEntries = std::vector<Any>;

  inline auto replace (
    const std::string &source,
    const std::string &pattern,
    const std::string &value
  ) {
    return std::regex_replace(source, std::regex(pattern), value);
  }

  class Error : public std::invalid_argument {
    public:
      std::string name;
      std::string message;
      std::string location;

      Error (
        const std::string& name,
        const std::string& message,
        const std::string& location
      ) : std::invalid_argument(name + ": " + message + " (from " + location + ")") {
        this->name = name;
        this->message = message;
        this->location = location;
      }

      auto str () const {
        return std::string(name + ": " + message + " (from " + location + ")");
      }
  };

  enum class Type {
    Empty = -1,
    Any = 0,
    Null = 1,
    Object = 2,
    Array = 3,
    Boolean = 4,
    Number = 5,
    String = 6,
    Raw = 7
  };

  template <typename D, Type t> struct Value {
    public:
      Type type = t;
      D data;

      auto typeof () const {
        switch (this->type) {
          case Type::Empty: return std::string("empty");
          case Type::Raw: return std::string("raw");
          case Type::Any: return std::string("any");
          case Type::Array: return std::string("array");
          case Type::Boolean: return std::string("boolean");
          case Type::Number: return std::string("number");
          case Type::Null: return std::string("null");
          case Type::Object: return std::string("object");
          case Type::String: return std::string("string");
        }
      }

      auto isRaw() const { return this->type == Type::Raw; }
      auto isArray () const { return this->type == Type::Array; }
      auto isBoolean () const { return this->type == Type::Boolean; }
      auto isNumber () const { return this->type == Type::Number; }
      auto isNull () const { return this->type == Type::Null; }
      auto isObject () const { return this->type == Type::Object; }
      auto isString () const { return this->type == Type::String; }
      auto isEmpty () const { return this->type == Type::Empty; }
  };

  class Null : public Value<std::nullptr_t, Type::Null> {
    public:
      Null () {}
      Null (std::nullptr_t) : Null() {}
      std::nullptr_t value () const {
        return nullptr;
      }

      std::string str () const {
        return "null";
      }
  };

  extern Null null;

  class Any : public Value<void *, Type::Any> {
    public:
      std::shared_ptr<void> pointer = nullptr;

      Any () {
        this->pointer = nullptr;
        this->type = Type::Null;
      }

      ~Any () {
        this->pointer = nullptr;
        this->type = Type::Any;
      }

      Any (const Any &any) {
        this->pointer = any.pointer;
        this->type = any.type;
      }

      Any (Type type, std::shared_ptr<void> pointer) {
        this->type = type;
        this->pointer = pointer;
      }

      Any (std::nullptr_t);
      Any (const Null);
      Any (bool);
      Any (const Boolean);
      Any (int64_t);
      Any (uint64_t);
      Any (uint32_t);
      Any (int32_t);
      Any (double);
      #if defined(__APPLE__)
      Any (ssize_t);
      #endif
      Any (const Number);
      Any (const char);
      Any (const char *);
      Any (const std::string);
      Any (const String);
      Any (const Object);
      Any (const ObjectEntries);
      Any (const Array);
      Any (const ArrayEntries);
      Any (const Raw source);

      std::string str () const;

      template <typename T> T& as () const {
        auto ptr = this->pointer.get();

        if (ptr != nullptr && this->type != Type::Null) {
          return *reinterpret_cast<T *>(ptr);
        }

        throw Error("BadCastError", "cannot cast to null value", __PRETTY_FUNCTION__);
      }
  };

  class Raw : public Value<std::string, Type::Raw> {
    public:
      Raw (const Raw& raw) { this->data = raw.data; }
      Raw (const Raw* raw) { this->data = raw->data; }
      Raw (const std::string& source) { this->data = source; }

      const std::string str () const {
        return this->data;
      }
  };

  extern Any anyNull;

  inline const auto typeof (const Any& any) {
    return any.typeof();
  }

  class Object : public Value<ObjectEntries, Type::Object> {
    public:
      using Entries = ObjectEntries;
      Object () = default;
      Object (std::map<std::string, int> entries) {
        for (auto const &tuple : entries) {
          auto key = tuple.first;
          auto value = tuple.second;
          this->data.insert_or_assign(key, value);
        }
      }

      Object (std::map<std::string, bool> entries) {
        for (auto const &tuple : entries) {
          auto key = tuple.first;
          auto value = tuple.second;
          this->data.insert_or_assign(key, value);
        }
      }

      Object (std::map<std::string, double> entries) {
        for (auto const &tuple : entries) {
          auto key = tuple.first;
          auto value = tuple.second;
          this->data.insert_or_assign(key, value);
        }
      }

      Object (std::map<std::string, int64_t> entries) {
        for (auto const &tuple : entries) {
          auto key = tuple.first;
          auto value = tuple.second;
          this->data.insert_or_assign(key, value);
        }
      }

      Object (const Object::Entries entries) {
        for (const auto& tuple : entries) {
          auto key = tuple.first;
          auto value = tuple.second;
          this->data.insert_or_assign(key, value);
        }
      }

      Object (const Object& object) {
        this->data = object.value();
      }

      Object (const std::map<std::string, std::string> map) {
        for (const auto& tuple : map) {
          auto key = tuple.first;
          auto value = Any(tuple.second);
          this->data.insert_or_assign(key, value);
        }
      }

      std::string str () const;

      const Object::Entries value () const {
        return this->data;
      }

      Any& get (const std::string key) {
        if (this->data.find(key) != this->data.end()) {
          return this->data.at(key);
        }

        return anyNull;
      }

      void set (const std::string key, Any value) {
        this->data[key] = value;
      }

      bool has (const std::string& key) const {
        return this->data.find(key) != this->data.end();
      }

      Any operator [] (const std::string& key) const {
        if (this->data.find(key) != this->data.end()) {
          return this->data.at(key);
        }

        return nullptr;
      }

      Any &operator [] (const std::string& key) {
        return this->data[key];
      }

      auto size () const {
        return this->data.size();
      }
  };

  class Array : public Value<ArrayEntries, Type::Array> {
    public:
      using Entries = ArrayEntries;
      Array () = default;
      Array (const Array& array) {
        this->data = array.value();
      }

      Array (const Array::Entries entries) {
        for (const auto& value : entries) {
          this->data.push_back(value);
        }
      }

      std::string str () const;

      Array::Entries value () const {
        return this->data;
      }

      bool has (const unsigned int index) const {
        return this->data.size() < index;
      }

      auto size () const {
        return this->data.size();
      }

      Any get (const unsigned int index) const {
        if (index < this->data.size()) {
          return this->data.at(index);
        }

        return nullptr;
      }

      void set (const unsigned int index, Any value) {
        if (index >= this->data.size()) {
          this->data.resize(index + 1);
        }

        this->data[index] = value;
      }

      void push (Any value) {
        this->set(this->size(), value);
      }

      Any& pop () {
        if (this->size() == 0) {
          return anyNull;
        }

        auto& value = this->data.back();
        this->data.pop_back();
        return value;
      }

      Any operator [] (const unsigned int index) const {
        if (index >= this->data.size()) {
          return nullptr;
        }

        return this->data.at(index);
      }

      Any &operator [] (const unsigned int index) {
        if (index >= this->data.size()) {
          this->data.resize(index + 1);
        }

        return this->data.at(index);
      }
  };

  class Boolean : public Value<bool, Type::Boolean> {
    public:
      Boolean () = default;
      Boolean (const Boolean& boolean) {
        this->data = boolean.value();
      }

      Boolean (bool boolean) {
        this->data = boolean;
      }

      Boolean (int data) {
        this->data = data != 0;
      }

      Boolean (int64_t data) {
        this->data = data != 0;
      }

      Boolean (double data) {
        this->data = data != 0;
      }

      Boolean (void *data) {
        this->data = data != nullptr;
      }

      Boolean (std::string string) {
        this->data = string.size() > 0;
      }

      bool value () const {
        return this->data;
      }

      std::string str () const {
        return this->data ? "true" : "false";
      }
  };

  class Number : public Value<double, Type::Number> {
    public:
      Number () = default;
      Number (const Number& number) {
        this->data = number.value();
      }

      Number (double number) {
        this->data = number;
      }

      Number (char number) {
        this->data = (double) number;
      }

      Number (int number) {
        this->data = (double) number;
      }

      Number (int64_t number) {
        this->data = (double) number;
      }

      Number (bool number) {
        this->data = (double) number;
      }

      Number (const String& string);

      float value () const {
        return this->data;
      }

      std::string str () const;
  };

  class String : public Value<std::string, Type::String> {
    public:
      String () = default;
      String (const String& data) {
        this->data = std::string(data.value());
      }

      String (const std::string data) {
        this->data = data;
      }

      String (const char data) {
        this->data = std::string(1, data);
      }

      String (const char *data) {
        this->data = std::string(data);
      }

      String (const Any& any) {
        this->data = any.str();
      }

      String (const Number& number);

      String (const Boolean& boolean) {
        this->data = boolean.str();
      }

      std::string str () const {
        auto escaped = replace(this->data, "\"", "\\\"");
        return "\"" + replace(escaped, "\n", "\\n") + "\"";
      }

      std::string value () const {
        return this->data;
      }

      auto size () const {
        return this->data.size();
      }
  };
}

#endif
