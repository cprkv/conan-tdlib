#include <td/telegram/Client.h>
#include <td/telegram/Log.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

// overloaded
namespace detail
{
template <class... Fs>
struct overload;

template <class F>
struct overload<F> : public F
{
  explicit overload(F f) : F(f)
  {
  }
};
template <class F, class... Fs>
struct overload<F, Fs...>
    : public overload<F>, overload<Fs...>
{
  overload(F f, Fs... fs) : overload<F>(f), overload<Fs...>(fs...)
  {
  }
  using overload<F>::operator();
  using overload<Fs...>::operator();
};
} // namespace detail

template <class... F>
auto overloaded(F... f)
{
  return detail::overload<F...>(f...);
}

namespace td_api = td::td_api;

class TdExample
{
public:
  TdExample()
  {
    td::Log::set_verbosity_level(1);
    client_ = std::make_unique<td::Client>();

    auto type = td_api::make_object<td_api::proxyTypeSocks5>();
    type->username_ = "telegram";
    type->password_ = "telegram";

    auto proxy = td_api::make_object<td_api::addProxy>();
    proxy->enable_ = true;
    proxy->server_ = "rknsoset.electrolysis.name";
    proxy->port_ = 1080;
    proxy->type_ = std::move(type);

    send_query(std::move(proxy), [](auto object) {
      if (object->get_id() == td_api::error::ID)
      {
        auto error = td::move_tl_object_as<td_api::error>(object);
        std::cerr << "addProxy() Error: " << to_string(error) << std::endl;
        abort();
      }
      else
      {
        std::cerr << "addProxy() OK" << std::endl;
      }
    });
  }

  void loop()
  {
    while (true)
    {
      if (need_restart_)
      {
        return;
      }
      else if (!are_authorized_)
      {
        process_response(client_->receive(10));
      }
      else
      {
        return;
      }
    }
  }

private:
  using Object = td_api::object_ptr<td_api::Object>;
  std::unique_ptr<td::Client> client_;

  td_api::object_ptr<td_api::AuthorizationState> authorization_state_;
  bool are_authorized_{false};
  bool need_restart_{false};
  std::uint64_t current_query_id_{0};
  std::uint64_t authentication_query_id_{0};

  std::map<std::uint64_t, std::function<void(Object)>> handlers_;
  std::map<std::int32_t, td_api::object_ptr<td_api::user>> users_;
  std::map<std::int64_t, std::string> chat_title_;

  void restart()
  {
    client_.reset();
    *this = TdExample();
  }

  void send_query(td_api::object_ptr<td_api::Function> f, std::function<void(Object)> handler)
  {
    auto query_id = next_query_id();
    if (handler)
    {
      handlers_.emplace(query_id, std::move(handler));
    }
    client_->send({query_id, std::move(f)});
  }

  void process_response(td::Client::Response response)
  {
    if (!response.object)
    {
      return;
    }
    //std::cerr << response.id << " " << to_string(response.object) << std::endl;
    if (response.id == 0)
    {
      return process_update(std::move(response.object));
    }
    auto it = handlers_.find(response.id);
    if (it != handlers_.end())
    {
      it->second(std::move(response.object));
    }
  }

  std::string get_user_name(std::int32_t user_id)
  {
    auto it = users_.find(user_id);
    if (it == users_.end())
    {
      return "unknown user";
    }
    return it->second->first_name_ + " " + it->second->last_name_;
  }

  void process_update(td_api::object_ptr<td_api::Object> update)
  {
    td_api::downcast_call(
        *update, overloaded(
                     [this](td_api::updateAuthorizationState &update_authorization_state) {
                       authorization_state_ = std::move(update_authorization_state.authorization_state_);
                       on_authorization_state_update();
                     },
                     [this](td_api::updateNewChat &update_new_chat) {
                       chat_title_[update_new_chat.chat_->id_] = update_new_chat.chat_->title_;
                     },
                     [this](td_api::updateChatTitle &update_chat_title) {
                       chat_title_[update_chat_title.chat_id_] = update_chat_title.title_;
                     },
                     [this](td_api::updateUser &update_user) {
                       auto user_id = update_user.user_->id_;
                       users_[user_id] = std::move(update_user.user_);
                     },
                     [this](td_api::updateNewMessage &update_new_message) {
                       auto chat_id = update_new_message.message_->chat_id_;
                       auto sender_user_name = get_user_name(update_new_message.message_->sender_user_id_);
                       std::string text;
                       if (update_new_message.message_->content_->get_id() == td_api::messageText::ID)
                       {
                         text = static_cast<td_api::messageText &>(*update_new_message.message_->content_).text_->text_;
                       }
                       std::cerr << "Got message: [chat_id:" << chat_id << "] [from:" << sender_user_name << "] ["
                                 << text << "]" << std::endl;
                     },
                     [](auto &update) {}));
  }

  auto create_authentication_query_handler()
  {
    return [this, id = authentication_query_id_](Object object) {
      if (id == authentication_query_id_)
      {
        check_authentication_error(std::move(object));
      }
    };
  }

  void on_authorization_state_update()
  {
    authentication_query_id_++;
    td_api::downcast_call(
        *authorization_state_,
        overloaded(
            [this](td_api::authorizationStateReady &) {
              are_authorized_ = true;
              std::cerr << "Got authorization" << std::endl;
            },
            [this](td_api::authorizationStateLoggingOut &) {
              are_authorized_ = false;
              std::cerr << "Logging out" << std::endl;
            },
            [this](td_api::authorizationStateClosing &) { std::cerr << "Closing" << std::endl; },
            [this](td_api::authorizationStateClosed &) {
              are_authorized_ = false;
              need_restart_ = true;
              std::cerr << "Terminated" << std::endl;
            },
            [this](td_api::authorizationStateWaitCode &wait_code) {
              std::string first_name;
              std::string last_name;
              if (!wait_code.is_registered_)
              {
                std::cerr << "Enter your first name: ";
                std::cin >> first_name;
                std::cerr << "Enter your last name: ";
                std::cin >> last_name;
              }
              std::cerr << "Enter authentication code: ";
              std::string code;
              std::cin >> code;
              send_query(td_api::make_object<td_api::checkAuthenticationCode>(code, first_name, last_name),
                         create_authentication_query_handler());
            },
            [this](td_api::authorizationStateWaitPassword &) {
              std::cerr << "Enter authentication password: ";
              std::string password;
              std::cin >> password;
              send_query(td_api::make_object<td_api::checkAuthenticationPassword>(password),
                         create_authentication_query_handler());
            },
            [this](td_api::authorizationStateWaitPhoneNumber &) {
              std::cerr << "Enter phone number: ";
              std::string phone_number;
              std::cin >> phone_number;
              send_query(td_api::make_object<td_api::setAuthenticationPhoneNumber>(
                             phone_number, false /*allow_flash_calls*/, false /*is_current_phone_number*/),
                         create_authentication_query_handler());
            },
            [this](td_api::authorizationStateWaitEncryptionKey &) {
              need_restart_ = true;
            },
            [this](td_api::authorizationStateWaitTdlibParameters &) {
              auto parameters = td_api::make_object<td_api::tdlibParameters>();
              parameters->database_directory_ = "tdlib";
              parameters->use_message_database_ = true;
              parameters->use_secret_chats_ = true;
              parameters->api_id_ = 94575;
              parameters->api_hash_ = "a3406de8d171bb422bb6ddf3bbd800e2";
              parameters->system_language_code_ = "en";
              parameters->device_model_ = "Desktop";
              parameters->system_version_ = "Unknown";
              parameters->application_version_ = "1.0";
              parameters->enable_storage_optimizer_ = true;
              send_query(td_api::make_object<td_api::setTdlibParameters>(std::move(parameters)),
                         create_authentication_query_handler());
            }));
  }

  void check_authentication_error(Object object)
  {
    if (object->get_id() == td_api::error::ID)
    {
      auto error = td::move_tl_object_as<td_api::error>(object);
      std::cerr << "Error: " << to_string(error);
      on_authorization_state_update();
    }
  }

  std::uint64_t next_query_id()
  {
    return ++current_query_id_;
  }
};

int main()
{
  TdExample example;
  example.loop();
  return 0;
}