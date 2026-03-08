#pragma once

#include "json.h"
#include <optional>
#include <vector>
#include <variant>
#include <string>

namespace json {

class Builder;
class DictItemContext;
class DictValueContext;
class ArrayItemContext;

// Базовый класс для хранения ссылки на Builder
class BaseContext {
protected:
    Builder& builder_;
public:
    BaseContext(Builder& builder) : builder_(builder) {}
};

// Контекст для методов, которые могут вызывать StartDict и StartArray
class ItemContext : public BaseContext {
public:
    ItemContext(Builder& builder) : BaseContext(builder) {}
    DictItemContext StartDict();
    ArrayItemContext StartArray();
};

// Контекст внутри словаря (ожидаем Key или EndDict)
class DictItemContext : public BaseContext {
public:
    DictItemContext(Builder& builder) : BaseContext(builder) {}
    DictValueContext Key(std::string key);
    Builder& EndDict();
};

// Контекст после ввода ключа (ожидаем Value, StartDict или StartArray)
class DictValueContext : public ItemContext {
public:
    DictValueContext(Builder& builder) : ItemContext(builder) {}
    DictItemContext Value(Node::Value value);
};

// Контекст массива (ожидаем Value, StartDict, StartArray или EndArray)
class ArrayItemContext : public ItemContext {
public:
    ArrayItemContext(Builder& builder) : ItemContext(builder) {}
    ArrayItemContext Value(Node::Value value);
    Builder& EndArray();
};

class Builder {
public:
    Builder();
    
    Node Build();
    DictValueContext Key(std::string key);
    Builder& Value(Node::Value value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndDict();
    Builder& EndArray();

private:
    Node root_;
    struct ArrayItem {
        Array* array;
        size_t index;
    };
    std::vector<std::variant<Dict*, Array*, ArrayItem>> stack_;
    std::optional<std::string> current_key_;
    bool started_ = false;
    bool is_built_ = false;

    void CheckNotBuilt() const;
    void CheckNotCompleted() const;
};

// Реализация inline-методов контекстов 
// (делегирование вызовов реальному Builder)

inline DictItemContext ItemContext::StartDict() {
    return builder_.StartDict();
}

inline ArrayItemContext ItemContext::StartArray() {
    return builder_.StartArray();
}

inline DictValueContext DictItemContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

inline Builder& DictItemContext::EndDict() {
    return builder_.EndDict();
}

inline DictItemContext DictValueContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return DictItemContext(builder_);
}

inline ArrayItemContext ArrayItemContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return ArrayItemContext(builder_);
}

inline Builder& ArrayItemContext::EndArray() {
    return builder_.EndArray();
}

} // namespace json
