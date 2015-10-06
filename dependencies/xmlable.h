/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      XMLable   abstract interface for xmlable objects
 *      XMLableDB container of xmlable objects
 *
 ******************************************************************************/

#ifndef XMLABLE2_H
#define XMLABLE2_H

#include <string>
#include <list>
#include <boost/type_traits.hpp>
#include "pugixml.hpp"
#include "custom_logger.h"

class XMLable {
 public:
  XMLable() {}

  virtual void to_xml(pugi::xml_node &node) const = 0;
  virtual void from_xml(const pugi::xml_node &node) = 0;
  virtual std::string xml_element_name() const = 0;

  ///////these functions needed by XMLableDB///////////
  //virtual bool operator== (const XMLable&) const = 0;
  //virtual bool shallow_equals(const XMLable&) const = 0;

};

template<typename T>
class XMLableDB : public XMLable {
public:
  XMLableDB(std::string xml_name) {xml_name_ = xml_name;}
  //  std::string name() const {return name_;}
  std::string xml_element_name() const {return xml_name_;}

  bool empty() const {return my_data_.empty();}
  int size() const {return my_data_.size();}
  void clear() {my_data_.clear();}

  bool operator!= (const XMLableDB& other) const {return !operator==(other);}
  bool operator== (const XMLableDB& other) const {
    if (xml_name_ != other.xml_name_)
      return false;
    if (my_data_.size() != other.my_data_.size())
      return false;
    //ordering does not matter, deep compare on elements
    for (auto &q : my_data_)
      if (!other.has(q))
        return false;
    return true;
  }

  bool shallow_equals(const XMLableDB& other) const {
    return (xml_name_ == other.xml_name_);
    //&& (name_ == other.name_));
  }

  bool has(const T& t) const {
    for (auto &q : my_data_)
      if (q == t)
        return true;
    return false;
  }

  bool has_a(const T& t) const {
    for (auto &q : my_data_)
      if (t.shallow_equals(q))
        return true;
    return false;
  }
  
  void add(T t) {
    if (t == T())
      return;
    if (!has_a(t))
      my_data_.push_back(t);
  }

  void add_a(T t) {
    if (t == T())
      return;
    my_data_.push_back(t);
  }

  
  void replace(T t) {
    if (t == T())
      return;
    bool replaced = false;
    for (auto &q : my_data_)
      if (q.shallow_equals(t)) {
        replaced = true;
        q = t;
      }
    if (!replaced)
      my_data_.push_back(t);
  }
 
  void remove(const T &t) {  //using deep compare
    typename std::list<T>::iterator it = my_data_.begin();
    while (it != my_data_.end()) {
      if (*it == t)
        it = my_data_.erase(it);
      ++it;
    }
  }

  void remove_a(const T &t) {  //using shallow compare
    typename std::list<T>::iterator it = my_data_.begin();
    while (it != my_data_.end()) {
      if (it->shallow_equals(t))
        it = my_data_.erase(it);
      ++it;
    }
  }
  
  void remove(int i) {
    if (i < size()) {
      typename std::list<T>::iterator it = std::next(my_data_.begin(), i);
      my_data_.erase(it);
    }
  }
    
  T get(T t) const {
    for (auto &q: my_data_)
      if (q.shallow_equals(t))
        return q;
    return T();    
  }
  
  T get(int i) const {
    if (i < size()) {
      typename std::list<T>::const_iterator it = std::next(my_data_.begin(), i);
      return *it;
    }
    return T();    
  }
  
  void up(int i) {
    if (i && (i < size())) {
      typename std::list<T>::iterator it = std::next(my_data_.begin(), i-1);
      std::swap( *it, *std::next( it ) );
    }
  }
    
  void down(int i) {
    if ((i+1) < size()) {
      typename std::list<T>::iterator it = std::next(my_data_.begin(), i);
      std::swap( *it, *std::next( it ) );
    }
  }

  void write_xml(std::string file_name) const {
    pugi::xml_document doc;
    pugi::xml_node root = doc.root();
    this->to_xml(root);
    doc.save_file(file_name.c_str());
  }
  
  void read_xml(std::string file_name) {
    pugi::xml_document doc;
    if (!doc.load_file(file_name.c_str()))
      return;


    pugi::xml_node root = doc.child(xml_name_.c_str());

    if (root) {
      this->from_xml(root);
    }
  }

  void to_xml(pugi::xml_node &node) const  {
    pugi::xml_node child = node.append_child(xml_name_.c_str());
    for (auto &q : my_data_)
      q.to_xml(child);
  }

  void from_xml(const pugi::xml_node &node) {
    if (node.name() != xml_name_)
      return;
    for (pugi::xml_node &child : node.children()) {
      T t;
      t.from_xml(child);
      if (!has(t))  // if doesn't have it, append, no duplicates
        my_data_.push_back(t);
    }
  }

  std::vector<T> to_vector() const {
    if (!my_data_.empty())
      return std::vector<T>(my_data_.begin(), my_data_.end());
    else
      return std::vector<T>();
  }

  void from_vector(std::vector<T> vec) {
    my_data_.clear();
    if (!vec.empty())
      my_data_ = std::list<T>(vec.begin(), vec.end());
  }


  std::list<T> my_data_;

// protected:
  std::string xml_name_;
};


#endif
