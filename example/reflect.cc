#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cinttypes>

#include "wasm.hh"


auto operator<<(std::ostream& out, wasm::Mutability mut) -> std::ostream& {
  switch (mut) {
    case wasm::VAR: return out << "var";
    case wasm::CONST: return out << "const";
  }
  return out;
}

auto operator<<(std::ostream& out, wasm::Limits limits) -> std::ostream& {
  out << limits.min;
  if (limits.max < wasm::Limits(0).max) out << " " << limits.max;
  return out;
}

auto operator<<(std::ostream& out, const wasm::ValType& type) -> std::ostream& {
  switch (type.kind()) {
    case wasm::I32: return out << "i32";
    case wasm::I64: return out << "i64";
    case wasm::F32: return out << "f32";
    case wasm::F64: return out << "f64";
    case wasm::ANYREF: return out << "anyref";
    case wasm::FUNCREF: return out << "funcref";
  }
  return out;
}

auto operator<<(std::ostream& out, const wasm::vec<wasm::ValType*>& types) -> std::ostream& {
  bool first = true;
  for (size_t i = 0; i < types.size(); ++i) {
    if (first) {
      first = false;
    } else {
      out << " ";
    }
    out << *types[i].get();
  }
  return out;
}

auto operator<<(std::ostream& out, const wasm::ExternType& type) -> std::ostream& {
  switch (type.kind()) {
    case wasm::EXTERN_FUNC: {
      out << "func " << type.func()->params() << " -> " << type.func()->results();
    } break;
    case wasm::EXTERN_GLOBAL: {
      out << "global " << type.global()->mutability() << " " << *type.global()->content();
    } break;
    case wasm::EXTERN_TABLE: {
      out << "table " << type.table()->limits() << " " << *type.table()->element();
    } break;
    case wasm::EXTERN_MEMORY: {
      out << "memory " << type.memory()->limits();
    } break;
  }
  return out;
}

auto operator<<(std::ostream& out, const wasm::Name& name) -> std::ostream& {
  out << "\"" << std::string(name.get(), name.size()) << "\"";
  return out;
}


void run(int argc, const char* argv[]) {
  // Initialize.
  std::cout << "Initializing..." << std::endl;
  auto engine = wasm::Engine::make(argc, argv);
  auto store = wasm::Store::make(engine);

  // Load binary.
  std::cout << "Loading binary..." << std::endl;
  std::ifstream file("reflect.wasm");
  file.seekg(0, std::ios_base::end);
  auto file_size = file.tellg();
  file.seekg(0);
  auto binary = wasm::vec<byte_t>::make_uninitialized(file_size);
  file.read(binary.get(), file_size);
  file.close();
  if (file.fail()) {
    std::cout << "> Error loading module!" << std::endl;
    return;
  }

  // Compile.
  std::cout << "Compiling module..." << std::endl;
  auto module = wasm::Module::make(store, binary);
  if (!module) {
    std::cout << "> Error compiling module!" << std::endl;
    return;
  }

  // Instantiate.
  std::cout << "Instantiating module..." << std::endl;
  auto imports = wasm::vec<wasm::Extern*>::make();
  auto instance = wasm::Instance::make(store, module, imports);
  if (!instance) {
    std::cout << "> Error instantiating module!" << std::endl;
    return;
  }

  // Extract exports.
  std::cout << "Extracting export..." << std::endl;
  auto export_types = module->exports();
  auto exports = instance->exports();
  assert(exports.size() == export_types.size());

  for (size_t i = 0; i < exports.size(); ++i) {
    assert(exports[i]->kind() == export_types[i]->type()->kind());
    std::cout << "> export " << i << " " << export_types[i]->name() << std::endl;
    std::cout << ">> initial: " << *export_types[i]->type() << std::endl;
    std::cout << ">> current: " << *exports[i]->type() << std::endl;
  }

  // Shut down.
  std::cout << "Shutting down..." << std::endl;
}


int main(int argc, const char* argv[]) {
  run(argc, argv);
  std::cout << "Done." << std::endl;
  return 0;
}
