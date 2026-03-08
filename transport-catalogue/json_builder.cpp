#include "json_builder.h"
#include <stdexcept>

namespace json {

Builder::Builder() : root_(nullptr) {}

void Builder::CheckNotBuilt() const {
    if (is_built_) throw std::logic_error("Object already built");
}

void Builder::CheckNotCompleted() const {
    if (started_ && stack_.empty()) throw std::logic_error("Object complete");
}

DictValueContext Builder::Key(std::string key) {
    CheckNotBuilt();
    CheckNotCompleted();
    if (stack_.empty()) throw std::logic_error("Key outside dict");
    
    auto& top = stack_.back();
    if (std::holds_alternative<Dict*>(top)) {
        if (current_key_) throw std::logic_error("Duplicate key");
        current_key_ = std::move(key);
    } 

    else if (std::holds_alternative<ArrayItem>(top)) {
        ArrayItem item = std::get<ArrayItem>(top);
        Node& node = (*item.array)[item.index];
        if (node.IsDict()) {
            if (current_key_) throw std::logic_error("Duplicate key");
            current_key_ = std::move(key);
        } else {
            throw std::logic_error("Key inside array");
        }
    } 
    else {
        throw std::logic_error("Key inside array");
    }
    return DictValueContext(*this);
}

Builder& Builder::Value(Node::Value value) {
    CheckNotBuilt();
    CheckNotCompleted();
    if (stack_.empty()) {
        root_ = Node(std::move(value));
        started_ = true;
        return *this;
    }
    auto& top = stack_.back();
    if (std::holds_alternative<Dict*>(top)) {
        if (!current_key_) throw std::logic_error("Value without key");
        Dict* dict = std::get<Dict*>(top);
        (*dict)[*current_key_] = Node(std::move(value));
        current_key_.reset();
    } else if (std::holds_alternative<Array*>(top)) {
        Array* arr = std::get<Array*>(top);
        arr->emplace_back(std::move(value));
    } else if (std::holds_alternative<ArrayItem>(top)) {
        ArrayItem item = std::get<ArrayItem>(top);
        Node& node = (*item.array)[item.index];
        if (node.IsArray()) {
            node.AsArray().emplace_back(std::move(value));
        } else if (node.IsDict()) {
            if (!current_key_) throw std::logic_error("Value without key");
            node.AsDict()[*current_key_] = Node(std::move(value));
            current_key_.reset();
        } else {
            throw std::logic_error("Unexpected node type");
        }
    }
    return *this;
}

DictItemContext Builder::StartDict() {
    CheckNotBuilt();
    CheckNotCompleted();
    if (stack_.empty()) {
        root_ = Node(Dict{});
        stack_.push_back(&root_.AsDict());
        started_ = true;
        return DictItemContext(*this);
    }
    auto& top = stack_.back();
    if (std::holds_alternative<Dict*>(top)) {
        if (!current_key_) throw std::logic_error("StartDict without key");
        Dict* dict = std::get<Dict*>(top);
        (*dict)[*current_key_] = Node(Dict{});
        Dict* new_dict = &(*dict)[*current_key_].AsDict();
        stack_.push_back(new_dict);
        current_key_.reset();
    } else if (std::holds_alternative<Array*>(top)) {
        Array* arr = std::get<Array*>(top);
        arr->emplace_back(Dict{});
        size_t idx = arr->size() - 1;
        stack_.push_back(ArrayItem{arr, idx});
    } else if (std::holds_alternative<ArrayItem>(top)) {
        ArrayItem item = std::get<ArrayItem>(top);
        Node& node = (*item.array)[item.index];
        if (node.IsArray()) {
            node.AsArray().emplace_back(Dict{});
            size_t idx = node.AsArray().size() - 1;
            stack_.push_back(ArrayItem{&node.AsArray(), idx});
        } else if (node.IsDict()) {
            if (!current_key_) throw std::logic_error("StartDict without key");
            node.AsDict()[*current_key_] = Node(Dict{});
            Dict* new_dict = &node.AsDict()[*current_key_].AsDict();
            stack_.push_back(new_dict);
            current_key_.reset();
        } else {
            throw std::logic_error("Unexpected node type");
        }
    }
    return DictItemContext(*this);
}

ArrayItemContext Builder::StartArray() {
    CheckNotBuilt();
    CheckNotCompleted();
    if (stack_.empty()) {
        root_ = Node(Array{});
        stack_.push_back(&root_.AsArray());
        started_ = true;
        return ArrayItemContext(*this);
    }
    auto& top = stack_.back();
    if (std::holds_alternative<Dict*>(top)) {
        if (!current_key_) throw std::logic_error("StartArray without key");
        Dict* dict = std::get<Dict*>(top);
        (*dict)[*current_key_] = Node(Array{});
        Array* new_arr = &(*dict)[*current_key_].AsArray();
        stack_.push_back(new_arr);
        current_key_.reset();
    } else if (std::holds_alternative<Array*>(top)) {
        Array* arr = std::get<Array*>(top);
        arr->emplace_back(Array{});
        size_t idx = arr->size() - 1;
        stack_.push_back(ArrayItem{arr, idx});
    } else if (std::holds_alternative<ArrayItem>(top)) {
        ArrayItem item = std::get<ArrayItem>(top);
        Node& node = (*item.array)[item.index];
        if (node.IsArray()) {
            node.AsArray().emplace_back(Array{});
            size_t idx = node.AsArray().size() - 1;
            stack_.push_back(ArrayItem{&node.AsArray(), idx});
        } else if (node.IsDict()) {
            if (!current_key_) throw std::logic_error("StartArray without key");
            node.AsDict()[*current_key_] = Node(Array{});
            Array* new_arr = &node.AsDict()[*current_key_].AsArray();
            stack_.push_back(new_arr);
            current_key_.reset();
        } else {
            throw std::logic_error("Unexpected node type");
        }
    }
    return ArrayItemContext(*this);
}

Builder& Builder::EndDict() {
    CheckNotBuilt();
    CheckNotCompleted();
    if (stack_.empty()) throw std::logic_error("EndDict without StartDict");
    auto& top = stack_.back();
    if (std::holds_alternative<Dict*>(top)) {
        if (current_key_) throw std::logic_error("Unfinished key");
        stack_.pop_back();
    } else if (std::holds_alternative<ArrayItem>(top)) {
        ArrayItem item = std::get<ArrayItem>(top);
        Node& node = (*item.array)[item.index];
        if (node.IsDict()) {
            if (current_key_) throw std::logic_error("Unfinished key");
            stack_.pop_back();
        } else {
            throw std::logic_error("EndDict on non-dict");
        }
    } else {
        throw std::logic_error("EndDict in array context");
    }
    return *this;
}

Builder& Builder::EndArray() {
    CheckNotBuilt();
    CheckNotCompleted();
    if (stack_.empty()) throw std::logic_error("EndArray without StartArray");
    auto& top = stack_.back();
    if (std::holds_alternative<Array*>(top)) {
        stack_.pop_back();
    } else if (std::holds_alternative<ArrayItem>(top)) {
        ArrayItem item = std::get<ArrayItem>(top);
        Node& node = (*item.array)[item.index];
        if (node.IsArray()) {
            stack_.pop_back();
        } else {
            throw std::logic_error("EndArray on non-array");
        }
    } else {
        throw std::logic_error("EndArray in dict context");
    }
    return *this;
}

Node Builder::Build() {
    CheckNotBuilt();
    if (!started_ || !stack_.empty()) throw std::logic_error("Incomplete object");
    is_built_ = true;
    return std::move(root_);
}

} // namespace json
