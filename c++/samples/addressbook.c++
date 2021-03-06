// This sample code appears in the documentation for the C++ implementation.
//
// Compile with:
//   capnpc -oc++ addressbook.capnp
//   c++ -std=c++11 -Wall addressbook.c++ addressbook.capnp.c++ -lcapnp -o addressbook
//
// Run like:
//   ./addressbook write | ./addressbook read
// Use "dwrite" and "dread" to use dynamic code instead.

// TODO(test):  Needs cleanup.

#include "addressbook.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <iostream>

void writeAddressBook(int fd) {
  ::capnp::MallocMessageBuilder message;

  AddressBook::Builder addressBook = message.initRoot<AddressBook>();
  ::capnp::List<Person>::Builder people = addressBook.initPeople(2);

  Person::Builder alice = people[0];
  alice.setId(123);
  alice.setName("Alice");
  alice.setEmail("alice@example.com");
  // Type shown for explanation purposes; normally you'd use auto.
  ::capnp::List<Person::PhoneNumber>::Builder alicePhones =
      alice.initPhones(1);
  alicePhones[0].setNumber("555-1212");
  alicePhones[0].setType(Person::PhoneNumber::Type::MOBILE);
  alice.getEmployment().setSchool("MIT");

  Person::Builder bob = people[1];
  bob.setId(456);
  bob.setName("Bob");
  bob.setEmail("bob@example.com");
  auto bobPhones = bob.initPhones(2);
  bobPhones[0].setNumber("555-4567");
  bobPhones[0].setType(Person::PhoneNumber::Type::HOME);
  bobPhones[1].setNumber("555-7654");
  bobPhones[1].setType(Person::PhoneNumber::Type::WORK);
  bob.getEmployment().setUnemployed();

  writePackedMessageToFd(fd, message);
}

void printAddressBook(int fd) {
  ::capnp::PackedFdMessageReader message(fd);

  AddressBook::Reader addressBook = message.getRoot<AddressBook>();

  for (Person::Reader person : addressBook.getPeople()) {
    std::cout << person.getName().cStr() << ": "
              << person.getEmail().cStr() << std::endl;
    for (Person::PhoneNumber::Reader phone: person.getPhones()) {
      const char* typeName = "UNKNOWN";
      switch (phone.getType()) {
        case Person::PhoneNumber::Type::MOBILE: typeName = "mobile"; break;
        case Person::PhoneNumber::Type::HOME: typeName = "home"; break;
        case Person::PhoneNumber::Type::WORK: typeName = "work"; break;
      }
      std::cout << "  " << typeName << " phone: "
                << phone.getNumber().cStr() << std::endl;
    }
    Person::Employment::Reader employment = person.getEmployment();
    switch (employment.which()) {
      case Person::Employment::UNEMPLOYED:
        std::cout << "  unemployed" << std::endl;
        break;
      case Person::Employment::EMPLOYER:
        std::cout << "  employer: "
                  << employment.getEmployer().cStr() << std::endl;
        break;
      case Person::Employment::SCHOOL:
        std::cout << "  student at: "
                  << employment.getSchool().cStr() << std::endl;
        break;
      case Person::Employment::SELF_EMPLOYED:
        std::cout << "  self-employed" << std::endl;
        break;
    }
  }
}

#include "addressbook.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <iostream>
#include <capnp/schema.h>
#include <capnp/dynamic.h>

using ::capnp::DynamicValue;
using ::capnp::DynamicStruct;
using ::capnp::DynamicEnum;
using ::capnp::DynamicList;
using ::capnp::DynamicUnion;
using ::capnp::List;
using ::capnp::Schema;
using ::capnp::StructSchema;
using ::capnp::EnumSchema;

using ::capnp::Void;
using ::capnp::Text;
using ::capnp::MallocMessageBuilder;
using ::capnp::PackedFdMessageReader;

void dynamicWriteAddressBook(int fd, StructSchema schema) {
  // Write a message using the dynamic API to set each
  // field by text name.  This isn't something you'd
  // normally want to do; it's just for illustration.

  MallocMessageBuilder message;

  // Types shown for explanation purposes; normally you'd
  // use auto.
  DynamicStruct::Builder addressBook =
      message.initRoot<DynamicStruct>(schema);

  DynamicList::Builder people =
      addressBook.init("people", 2).as<DynamicList>();

  DynamicStruct::Builder alice =
      people[0].as<DynamicStruct>();
  alice.set("id", 123);
  alice.set("name", "Alice");
  alice.set("email", "alice@example.com");
  auto alicePhones = alice.init("phones", 1).as<DynamicList>();
  auto phone0 = alicePhones[0].as<DynamicStruct>();
  phone0.set("number", "555-1212");
  phone0.set("type", "mobile");
  alice.get("employment").as<DynamicUnion>()
       .set("school", "MIT");

  auto bob = people[1].as<DynamicStruct>();
  bob.set("id", 456);
  bob.set("name", "Bob");
  bob.set("email", "bob@example.com");

  // Some magic:  We can convert a dynamic sub-value back to
  // the native type with as<T>()!
  List<Person::PhoneNumber>::Builder bobPhones =
      bob.init("phones", 2).as<List<Person::PhoneNumber>>();
  bobPhones[0].setNumber("555-4567");
  bobPhones[0].setType(Person::PhoneNumber::Type::HOME);
  bobPhones[1].setNumber("555-7654");
  bobPhones[1].setType(Person::PhoneNumber::Type::WORK);
  bob.get("employment").as<DynamicUnion>()
     .set("unemployed", Void::VOID);

  writePackedMessageToFd(fd, message);
}

void dynamicPrintValue(DynamicValue::Reader value) {
  // Print an arbitrary message via the dynamic API by
  // iterating over the schema.  Look at the handling
  // of STRUCT in particular.

  switch (value.getType()) {
    case DynamicValue::VOID:
      std::cout << "";
      break;
    case DynamicValue::BOOL:
      std::cout << (value.as<bool>() ? "true" : "false");
      break;
    case DynamicValue::INT:
      std::cout << value.as<int64_t>();
      break;
    case DynamicValue::UINT:
      std::cout << value.as<uint64_t>();
      break;
    case DynamicValue::FLOAT:
      std::cout << value.as<double>();
      break;
    case DynamicValue::TEXT:
      std::cout << '\"' << value.as<Text>().cStr() << '\"';
      break;
    case DynamicValue::LIST: {
      std::cout << "[";
      bool first = true;
      for (auto element: value.as<DynamicList>()) {
        if (first) {
          first = false;
        } else {
          std::cout << ", ";
        }
        dynamicPrintValue(element);
      }
      std::cout << "]";
      break;
    }
    case DynamicValue::ENUM: {
      auto enumValue = value.as<DynamicEnum>();
      KJ_IF_MAYBE(enumerant, enumValue.getEnumerant()) {
        std::cout <<
            enumerant->getProto().getName().cStr();
      } else {
        // Unknown enum value; output raw number.
        std::cout << enumValue.getRaw();
      }
      break;
    }
    case DynamicValue::STRUCT: {
      std::cout << "(";
      auto structValue = value.as<DynamicStruct>();
      bool first = true;
      for (auto member:
           structValue.getSchema().getMembers()) {
        if (first) {
          first = false;
        } else {
          std::cout << ", ";
        }
        std::cout << member.getProto().getName().cStr()
                  << " = ";
        dynamicPrintValue(structValue.get(member));
      }
      std::cout << ")";
      break;
    }
    case DynamicValue::UNION: {
      auto unionValue = value.as<DynamicUnion>();
      KJ_IF_MAYBE(tag, unionValue.which()) {
        std::cout << tag->getProto().getName().cStr() << "(";
        dynamicPrintValue(unionValue.get());
        std::cout << ")";
      } else {
        // Unknown union member; must have come from newer
        // version of the protocol.
        std::cout << "?";
      }
      break;
    }
    default:
      // There are other types, we aren't handling them.
      std::cout << "?";
      break;
  }
}

void dynamicPrintMessage(int fd, StructSchema schema) {
  PackedFdMessageReader message(fd);
  dynamicPrintValue(message.getRoot<DynamicStruct>(schema));
  std::cout << std::endl;
}

int main(int argc, char* argv[]) {
  StructSchema schema = Schema::from<AddressBook>();
  if (argc != 2) {
    std::cerr << "Missing arg." << std::endl;
    return 1;
  } else if (strcmp(argv[1], "write") == 0) {
    writeAddressBook(1);
  } else if (strcmp(argv[1], "read") == 0) {
    printAddressBook(0);
  } else if (strcmp(argv[1], "dwrite") == 0) {
    dynamicWriteAddressBook(1, schema);
  } else if (strcmp(argv[1], "dread") == 0) {
    dynamicPrintMessage(0, schema);
  } else {
    std::cerr << "Invalid arg: " << argv[1] << std::endl;
    return 1;
  }
  return 0;
}

