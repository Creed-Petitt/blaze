#ifndef BLAZE_REPOSITORY_H
#define BLAZE_REPOSITORY_H

#include <blaze/database.h>
#include <blaze/exceptions.h>
#include <blaze/model.h>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <boost/describe.hpp>
#include <boost/core/demangle.hpp>

namespace blaze {

template <typename T>
class Repository;

/**
 * @brief Fluent Query Builder for Repository types.
 */
template <typename T>
class QueryBuilder {
private:
    Repository<T>& repo_;
    std::vector<std::string> conditions_;
    std::vector<std::string> params_;
    std::string order_by_;
    int limit_ = -1;
    int offset_ = -1;

public:
    explicit QueryBuilder(Repository<T>& repo) : repo_(repo) {}

    template <typename Val>
    QueryBuilder& where(const std::string& column, const std::string& op, Val&& val) {
        int idx = params_.size() + 1;
        conditions_.push_back("\"" + column + "\" " + op + " " + repo_.database()->placeholder(idx));
        params_.push_back(to_string_param(std::forward<Val>(val)));
        return *this;
    }

    QueryBuilder& order_by(const std::string& column, const std::string& direction = "ASC") {
        order_by_ = " ORDER BY \"" + column + "\" " + direction;
        return *this;
    }

    QueryBuilder& limit(int limit) {
        limit_ = limit;
        return *this;
    }

    QueryBuilder& offset(int offset) {
        offset_ = offset;
        return *this;
    }

    boost::asio::awaitable<std::vector<T>> all() {
        std::string sql = repo_.select_base();
        if (!conditions_.empty()) {
            sql += " WHERE ";
            for (size_t i = 0; i < conditions_.size(); ++i) {
                if (i > 0) sql += " AND ";
                sql += conditions_[i];
            }
        }
        sql += order_by_;
        if (limit_ != -1) sql += " LIMIT " + std::to_string(limit_);
        if (offset_ != -1) sql += " OFFSET " + std::to_string(offset_);

        co_return co_await repo_.database()->template query<T>(sql, params_);
    }

    boost::asio::awaitable<T> first() {
        this->limit(1);
        auto results = co_await all();
        if (results.empty()) {
            throw NotFound(repo_.table_name() + " not found");
        }
        co_return results[0];
    }
};

template <typename T, typename = void>
struct has_table_name : std::false_type {};

template <typename T>
struct has_table_name<T, std::void_t<decltype(T::table_name)>> : std::true_type {};

template <typename T>
class Repository {
protected:
    std::shared_ptr<Database> db_;
    std::string table_name_;

    // Helper to get table name from type
    std::string infer_table_name() {
        if constexpr (has_table_name<T>::value) {
            return std::string(T::table_name);
        }
        std::string name = boost::core::demangle(typeid(T).name());
        return name;
    }

    // Helper to get column list: "id, name, email" -> "\"id\", \"name\", \"email\""
    std::string get_columns() {
        std::ostringstream ss;
        using Members = boost::describe::describe_members<T, boost::describe::mod_any_access>;
        bool first = true;
        boost::mp11::mp_for_each<Members>([&](auto meta) {
            if (!first) ss << ", ";
            ss << "\"" << meta.name << "\"";
            first = false;
        });
        return ss.str();
    }

    // Helper to get primary key name (Assumes first field is PK)
    std::string get_pk_name() {
        std::string pk;
        using Members = boost::describe::describe_members<T, boost::describe::mod_any_access>;
        boost::mp11::mp_for_each<Members>([&](auto meta) {
            if (pk.empty()) pk = meta.name;
        });
        return pk;
    }

public:
    Repository(std::shared_ptr<Database> db) : db_(db) {
        table_name_ = infer_table_name();
    }

    // --- Accessors for QueryBuilder ---
    std::shared_ptr<Database> database() { return db_; }
    std::string table_name() const { return table_name_; }
    std::string select_base() { return "SELECT " + get_columns() + " FROM \"" + table_name_ + "\""; }

    /**
     * @brief Start a fluent query.
     * usage: co_await repo.query().where("age", ">", 20).limit(5).all();
     */
    QueryBuilder<T> query() {
        return QueryBuilder<T>(*this);
    }

    // Find by ID. Throws NotFound if missing.
    // usage: auto user = co_await users.find(1);
    template <typename ID>
    boost::asio::awaitable<T> find(ID id) {
        std::string pk = get_pk_name();
        // Safe: SELECT "id", "name" FROM "User" WHERE "id" = $1 (or ?)
        std::string sql = "SELECT " + get_columns() + " FROM \"" + table_name_ + "\" WHERE \"" + pk + "\" = " + db_->placeholder(1);
        
        auto results = co_await db_->query<T>(sql, id);
        if (results.empty()) {
            throw NotFound(table_name_ + " not found");
        }
        co_return results[0];
    }

    // Find all
    boost::asio::awaitable<std::vector<T>> all() {
        std::string sql = select_base();
        co_return co_await db_->query<T>(sql);
    }

    // Create (Insert)
    boost::asio::awaitable<void> save(const T& model) {
        std::ostringstream cols, vals;
        std::vector<std::string> params;
        int idx = 1;

        using Members = boost::describe::describe_members<T, boost::describe::mod_any_access>;
        bool first = true;
        
        boost::mp11::mp_for_each<Members>([&](auto meta) {
            if (!first) { cols << ", "; vals << ", "; }
            cols << "\"" << meta.name << "\"";
            vals << db_->placeholder(idx++);
            
            // Convert member to string param
            params.push_back(to_string_param(model.*meta.pointer));
            first = false;
        });

        std::string sql = "INSERT INTO \"" + table_name_ + "\" (" + cols.str() + ") VALUES (" + vals.str() + ")";
        co_await db_->query(sql, params);
    }

    // Update
    boost::asio::awaitable<void> update(const T& model) {
        std::ostringstream sets;
        std::vector<std::string> params;
        std::string pk_name = get_pk_name();
        std::string pk_value;
        int idx = 1;

        using Members = boost::describe::describe_members<T, boost::describe::mod_any_access>;
        bool first = true;

        boost::mp11::mp_for_each<Members>([&](auto meta) {
            std::string col = meta.name;
            std::string val = to_string_param(model.*meta.pointer);

            if (col == pk_name) {
                pk_value = val;
            } else {
                if (!first) sets << ", ";
                sets << "\"" << col << "\" = " << db_->placeholder(idx++);
                params.push_back(val);
                first = false;
            }
        });

        // Add PK to params last
        params.push_back(pk_value);
        std::string sql = "UPDATE \"" + table_name_ + "\" SET " + sets.str() + " WHERE \"" + pk_name + "\" = " + db_->placeholder(idx);
        
        co_await db_->query(sql, params);
    }

    // Delete
    template <typename ID>
    boost::asio::awaitable<void> remove(ID id) {
        std::string pk = get_pk_name();
        std::string sql = "DELETE FROM \"" + table_name_ + "\" WHERE \"" + pk + "\" = " + db_->placeholder(1);
        co_await db_->query(sql, id);
    }

    // Count
    boost::asio::awaitable<int> count() {
        std::string sql = "SELECT COUNT(*) FROM \"" + table_name_ + "\"";
        auto res = co_await db_->query(sql);
        if (res.empty()) co_return 0;
        // Parse "count" or "COUNT(*)" column (Index 0)
        std::string val = res[0][0].template as<std::string>();
        co_return val.empty() ? 0 : std::stoi(val);
    }

    // Flexible Finder: find_where("age > $1", 18)
    template<typename... Args>
    boost::asio::awaitable<std::vector<T>> find_where(const std::string& condition, Args&&... args) {
        std::string sql = "SELECT " + get_columns() + " FROM \"" + table_name_ + "\" WHERE " + condition;
        co_return co_await db_->query<T>(sql, std::forward<Args>(args)...);
    }
};

} // namespace blaze

#endif
