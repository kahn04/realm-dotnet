#include <iostream>
#include <realm.hpp>
#include <realm/table.hpp>
#include <realm/commit_log.hpp>
#include <realm/lang_bind_helper.hpp>
#include "ffi.hpp"
#include "transact_log_handler.hpp"
#include "shared_realm.hpp"
#include "schema.hpp"

#ifdef WIN32
#define REALM_EXPORT __declspec( dllexport )
#else
#define REALM_EXPORT
#endif

using namespace realm;

namespace binding {
    realm::RealmDelegate* delegate_instance;
    void process_error(RealmError* realm_error);
}

static
void convert_exception_to_error(RealmError* error)
{
    try {
        throw;
    }
    catch (const std::exception& ex) {
        error->type = RealmErrorType::system;
        error->message = ex.what();
    }
    catch (...) {
        error->type = RealmErrorType::unknown;
        error->message = "Unknown exception thrown";
    }
}

template <class T>
struct Default {
    static T default_value() {
        return T{};
    }
};
template <>
struct Default<void> {
    static void default_value() {}
};

template <class F>
auto handle_errors(F&& func) -> decltype(func())
{
    using RetVal = decltype(func());
    try {
        return func();
    }
    catch (...) {
        RealmError* out_error = new RealmError;
        convert_exception_to_error(out_error);
        binding::process_error(out_error);
        return Default<RetVal>::default_value();
    }
}

extern "C" {

using namespace realm;

REALM_EXPORT ObjectSchema* object_schema_new(const char* name)
{
    std::cout << "Creating object schema '" << name << "'.\r\n";

    auto p = new ObjectSchema;
    p->name = name;
    return p;
}

REALM_EXPORT void object_schema_add_property(ObjectSchema* cls, const char* name, DataType type, const char* object_type,
                                      bool is_primary, bool is_indexed, bool is_nullable)
{
    Property p;
    p.name = name;
    p.type = static_cast<PropertyType>(type);
    p.object_type = object_type ? std::string(object_type) : std::string();
    p.is_primary = is_primary;
    p.is_indexed = is_indexed;
    p.is_nullable = is_nullable;
    cls->properties.push_back(std::move(p));
}

REALM_EXPORT Schema* schema_new(ObjectSchema* object_schemas, size_t len)
{
    return new Schema(std::vector<ObjectSchema>(object_schemas, object_schemas + len * sizeof(ObjectSchema)));
}

REALM_EXPORT SharedRealm* shared_realm_open(Schema* schema, const char* path, bool read_only, SharedGroup::DurabilityLevel durability,
                        const char* encryption_key)
{
    Realm::Config config;
    config.path = path;
    config.read_only = read_only;
    config.in_memory = durability != SharedGroup::durability_Full;

    //config.encryption_key = encryption_key;
    config.encryption_key = std::vector<char>(&encryption_key[0], &encryption_key[strlen(encryption_key)]);

    config.schema.reset(schema);
    return new SharedRealm{Realm::get_shared_realm(config)};
}

REALM_EXPORT void shared_realm_delete(SharedRealm* realm)
{
    delete realm;
}

REALM_EXPORT bool shared_realm_has_table(SharedRealm* realm, const char* name)
{
    Group* g = (*realm)->read_group();
    return g->has_table(name);
}

REALM_EXPORT void shared_realm_begin_transaction(SharedRealm* realm)
{
    handle_errors([&]() {
        (*realm)->begin_transaction();
    });
}

REALM_EXPORT void shared_realm_commit_transaction(SharedRealm* realm)
{
    handle_errors([&]() {
        (*realm)->commit_transaction();
    });
}

REALM_EXPORT void shared_realm_cancel_transaction(SharedRealm* realm)
{
    handle_errors([&]() {
        (*realm)->cancel_transaction();
    });
}

REALM_EXPORT bool shared_realm_is_in_transaction(SharedRealm* realm)
{
    return (*realm)->is_in_transaction();
}

REALM_EXPORT bool shared_realm_refresh(SharedRealm* realm)
{
    return (*realm)->refresh();
}

//REALM_EXPORT void shared_group_promote_to_write(SharedGroup* sg, ClientHistory* hist)
//{
//    handle_errors([&]() {
//	    //LangBindHelper::promote_to_write(*sg, *hist);
//        transaction::begin(*sg, *hist, binding::delegate_instance);
//    });
//}
//
//REALM_EXPORT SharedGroup::version_type shared_group_commit(SharedGroup* sg)
//{
//    return handle_errors([&]() {
//        return sg->commit();
//    });
//}
//
//REALM_EXPORT void shared_group_commit_and_continue_as_read(SharedGroup* sg, ClientHistory* hist)
//{
//    //LangBindHelper::commit_and_continue_as_read(*sg);
//    transaction::commit(*sg, *hist, binding::delegate_instance);
//}
//
//REALM_EXPORT void shared_group_rollback(SharedGroup* sg)
//{
//    sg->rollback();
//}
//
//REALM_EXPORT void shared_group_rollback_and_continue_as_read(SharedGroup* sg, ClientHistory* hist)
//{
//    handle_errors([&]() {
//        //LangBindHelper::rollback_and_continue_as_read(*sg, *hist);
//        transaction::cancel(*sg, *hist, binding::delegate_instance);
//    });
//}
//
//REALM_EXPORT void table_retain(const Table* table)
//{
//    LangBindHelper::bind_table_ptr(table);
//}
//
//REALM_EXPORT void table_release(const Table* table)
//{
//    LangBindHelper::unbind_table_ptr(table);
//}
//
//REALM_EXPORT const Table* shared_group_get_table(SharedGroup* sg, const char* name)
//{
//    using sgf = _impl::SharedGroupFriend;
//    return handle_errors([&]() {
//        auto& group = sgf::get_group(*sg);
//        return LangBindHelper::get_table(group, name);
//    });
//}
//
//REALM_EXPORT Table* shared_group_get_table_mut(SharedGroup* sg, const char* name)
//{
//    using sgf = _impl::SharedGroupFriend;
//    return handle_errors([&]() {
//        auto& group = sgf::get_group(*sg);
//        return LangBindHelper::get_table(group, name);
//    });
//}
//
//REALM_EXPORT Table* shared_group_add_table(SharedGroup* sg, const char* name)
//{
//    using sgf = _impl::SharedGroupFriend;
//    return handle_errors([&]() {
//        auto& group = sgf::get_group(*sg);
//        return LangBindHelper::add_table(group, name);
//    });
//}
//
//REALM_EXPORT size_t table_add_column(Table* table, DataType type, const char* name, bool nullable)
//{
//    return handle_errors([&]() {
//        return table->add_column(type, name, nullable);
//    });
//}
//
//REALM_EXPORT size_t table_add_empty_row(Table* table)
//{
//    return handle_errors([&]() {
//        return table->add_empty_row();
//    });
//}
//
//REALM_EXPORT int64_t table_get_int(const Table* table, size_t col_ndx, size_t row_ndx)
//{
//    return table->get_int(col_ndx, row_ndx);
//}
//
//REALM_EXPORT void table_set_int(Table* table, size_t col_ndx, size_t row_ndx, int64_t value)
//{
//    return handle_errors([&]() {
//        table->set_int(col_ndx, row_ndx, value);
//    });
//}
//
//REALM_EXPORT void row_delete(Table::RowExpr* expr) {
//    delete expr;
//}
//
//REALM_EXPORT int64_t row_get_int(const Table::RowExpr* expr, size_t col_ndx) {
//    return expr->get_int(col_ndx);
//}

}
