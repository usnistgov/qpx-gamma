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

#ifndef XMLABLE_H
#define XMLABLE_H

#include <string>
#include <list>
#include <boost/type_traits.hpp>
#include "tinyxml2.h"
#include "custom_logger.h"

class XMLable {
 public:
  XMLable() {}

  virtual void to_xml(tinyxml2::XMLPrinter&) const = 0;
  virtual void from_xml(tinyxml2::XMLElement*) = 0;
  virtual std::string xml_element_name() const = 0;

  //fns such as these needed by XMLableDB
  //bool shallow_equals(const XMLable&) const = 0;
  //virtual bool operator== (const XMLable&) const = 0;
  //virtual bool shallow_equals(const XMLable&) const = 0;
  
};

template<class T>
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
    //    if (name_ != other.name_)
    //      return false;
    if (my_data_.size() != other.my_data_.size())
      return false;
    //ordering does not matter, deep compare on elements
    for (auto &q : my_data_)
      if (!other.has(q))
        return false;
    return true;
  };

  bool shallow_equals(const XMLableDB& other) const {
    return (xml_name_ == other.xml_name_);
    //&& (name_ == other.name_));
  };  

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
    for (auto &q : my_data_)
      if (q.shallow_equals(t))
        q = t;
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
    FILE* myfile;
    myfile = fopen (file_name.c_str(), "w");
    if (myfile == NULL)
      return;
    tinyxml2::XMLPrinter printer(myfile);
    printer.PushHeader(true, true);
    to_xml(printer);
    fclose(myfile);
  }
  
  void read_xml(std::string file_name) {
    FILE* myfile;
    myfile = fopen (file_name.c_str(), "r");
    if (myfile == nullptr)
      return;
    tinyxml2::XMLDocument docx;
    docx.LoadFile(myfile);
    tinyxml2::XMLElement* root = docx.FirstChildElement(xml_name_.c_str());
    if (root != nullptr)
      from_xml(root);
    fclose(myfile);
  }

  void to_xml(tinyxml2::XMLPrinter& printer) const {
    printer.OpenElement(xml_name_.c_str());
    for (auto &q : my_data_)
      q.to_xml(printer);
    printer.CloseElement();   
  }

  void from_xml(tinyxml2::XMLElement* root) {
    tinyxml2::XMLElement* node = root->FirstChildElement(T().xml_element_name().c_str());
    while (node != nullptr) {
      T t;
      t.from_xml(node);
      if (!has(t))  // if doesn't have it, append
        my_data_.push_back(t);
      node = dynamic_cast<tinyxml2::XMLElement*>(node->NextSibling());
    }    
  }
  
  std::list<T> my_data_;
  //  std::string name_;

 protected:
  std::string xml_name_;
};


#endif
