/*
 * Ashita kpacket Plugin - Copyright (c) 2025 Poroburu Development team
 * LGPL-3.0-or-later
 */

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <nlohmann/json.hpp>
#include <mutex>
#include <deque>
#include <thread>
#include <atomic>

struct PacketRow {
  std::string time_str;
  std::string topic;
  uint16_t id;
  std::string name;
  std::string dir;
  uint32_t size;
  uint64_t local_seq{0};
  uint64_t remote_msg_id{0};
  bool gap{false};
};

class MonitorModel {
 public:
  std::mutex mtx;
  std::deque<PacketRow> rows;
  std::atomic<bool> is_connected{false};
  std::atomic<bool> running{false};
  std::string sub_endpoint = "tcp://localhost:5555";
  std::string req_endpoint = "tcp://localhost:5556";
  size_t max_rows = 2000;
  std::atomic<uint64_t> total_packets{0};
  std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
  std::atomic<long long> last_packet_ms{0};
  std::atomic<long long> last_health_ok_ms{0};
  std::atomic<uint64_t> local_seq_counter{0};
  std::atomic<uint64_t> last_remote_msg_id{0};
  // Rate updated every 10 packets
  std::atomic<uint32_t> batch_count{0};
  std::atomic<long long> batch_start_ms{0};
  std::atomic<double> last_batch_rate{0.0};
};

static std::string NowHMS() {
  using namespace std::chrono;
  auto now = system_clock::now();
  auto t = system_clock::to_time_t(now);
  std::tm tm_buf;
#ifdef _WIN32
  localtime_s(&tm_buf, &t);
#else
  localtime_r(&t, &tm_buf);
#endif
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
  return buf;
}

void SubscriberThread(MonitorModel* model, ftxui::ScreenInteractive* screen) {
  try {
    zmq::context_t ctx(1);
    zmq::socket_t sub(ctx, ZMQ_SUB);
    sub.connect(model->sub_endpoint);
    sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    sub.set(zmq::sockopt::rcvtimeo, 250);
    while (model->running) {
      zmq::multipart_t mp;
      if (!mp.recv(sub)) { continue; }
      if (mp.size() < 2) continue;
      std::string topic = mp[0].to_string();
      std::string meta_json = mp[1].to_string();
      auto meta = nlohmann::json::parse(meta_json, nullptr, false);
      if (meta.is_discarded()) continue;
      model->is_connected.store(true, std::memory_order_relaxed);
      auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
      model->last_packet_ms.store(now_ms, std::memory_order_relaxed);
      // Batch-based rate computation (every 10 packets)
      uint32_t cnt = model->batch_count.fetch_add(1, std::memory_order_relaxed) + 1;
      if (cnt == 1) {
        model->batch_start_ms.store(now_ms, std::memory_order_relaxed);
      } else if (cnt >= 10) {
        long long start_ms = model->batch_start_ms.load(std::memory_order_relaxed);
        long long elapsed = now_ms - start_ms;
        if (elapsed > 0) {
          double rate = (10.0 * 1000.0) / static_cast<double>(elapsed);
          model->last_batch_rate.store(rate, std::memory_order_relaxed);
        }
        model->batch_count.store(0, std::memory_order_relaxed);
        model->batch_start_ms.store(now_ms, std::memory_order_relaxed);
      }
      PacketRow row;
      row.time_str = NowHMS();
      row.topic = topic;
      row.id = meta.value("packet_id", 0);
      row.name = meta.value("packet_name", "");
      row.dir = meta.value("direction", "");
      row.size = meta.value("size", 0);
      row.local_seq = model->local_seq_counter.fetch_add(1, std::memory_order_relaxed) + 1;
      row.remote_msg_id = meta.value("message_id", static_cast<uint64_t>(0));
      {
        uint64_t prev = model->last_remote_msg_id.load(std::memory_order_relaxed);
        if (prev != 0 && row.remote_msg_id != 0 && row.remote_msg_id != prev + 1) {
          row.gap = true;
        }
        if (row.remote_msg_id != 0) {
          model->last_remote_msg_id.store(row.remote_msg_id, std::memory_order_relaxed);
        }
      }
      {
        std::lock_guard<std::mutex> lk(model->mtx);
        model->rows.emplace_front(std::move(row));
        if (model->rows.size() > model->max_rows) model->rows.pop_back();
      }
      model->total_packets.fetch_add(1, std::memory_order_relaxed);
      if (screen) { screen->PostEvent(ftxui::Event::Custom); screen->RequestAnimationFrame(); }
    }
  } catch (...) {
    model->is_connected.store(false, std::memory_order_relaxed);
  }
}

void HealthThread(MonitorModel* model, ftxui::ScreenInteractive* screen) {
  try {
    zmq::context_t ctx(1);
    zmq::socket_t req(ctx, ZMQ_REQ);
    req.connect(model->req_endpoint);
    req.set(zmq::sockopt::sndtimeo, 500);
    req.set(zmq::sockopt::rcvtimeo, 500);
    int consecutive_fail = 0;
    while (model->running) {
      nlohmann::json cmd = { {"command", "status"}, {"params", nlohmann::json::object()} };
      auto payload = cmd.dump();
      zmq::message_t msg(payload.size());
      std::memcpy(msg.data(), payload.data(), payload.size());
      bool ok_send = req.send(msg, zmq::send_flags::none).has_value();
      zmq::message_t reply;
      bool ok_recv = ok_send && req.recv(reply, zmq::recv_flags::none).has_value();
      if (ok_recv) {
        model->is_connected.store(true, std::memory_order_relaxed);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();
        model->last_health_ok_ms.store(now_ms, std::memory_order_relaxed);
        consecutive_fail = 0;
      } else {
        consecutive_fail++;
        if (consecutive_fail >= 2) model->is_connected.store(false, std::memory_order_relaxed);
        if (consecutive_fail >= 3) {
          // Recreate socket to force reconnect
          try {
            req.close();
          } catch (...) {}
          req = zmq::socket_t(ctx, ZMQ_REQ);
          req.connect(model->req_endpoint);
          req.set(zmq::sockopt::sndtimeo, 500);
          req.set(zmq::sockopt::rcvtimeo, 500);
        }
      }
      if (screen) { screen->PostEvent(ftxui::Event::Custom); screen->RequestAnimationFrame(); }
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  } catch (...) {
    model->is_connected.store(false, std::memory_order_relaxed);
    if (screen) screen->PostEvent(ftxui::Event::Custom);
  }
}

void UiTickThread(std::atomic<bool>* running, ftxui::ScreenInteractive* screen) {
  while (running->load(std::memory_order_relaxed)) {
    if (screen) { screen->PostEvent(ftxui::Event::Custom); screen->RequestAnimationFrame(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }
}

int main() {
  MonitorModel model;
  model.start_time = std::chrono::steady_clock::now();
  auto screen = ftxui::ScreenInteractive::Fullscreen();

  // Start threads
  model.running = true;
  std::thread sub_thread(SubscriberThread, &model, &screen);
  sub_thread.detach();
  std::thread health_thread(HealthThread, &model, &screen);
  health_thread.detach();
  std::thread ui_tick(UiTickThread, &model.running, &screen);
  ui_tick.detach();

  auto main = ftxui::Renderer([&] {
    // Header (single line)
    // Determine connection state: recent packets OR recent health success
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count();
    long long last = model.last_packet_ms.load(std::memory_order_relaxed);
    long long last_ok = model.last_health_ok_ms.load(std::memory_order_relaxed);
    bool recent = last > 0 && (now_ms - last) <= 2000; // 2s window
    bool recent_ok = last_ok > 0 && (now_ms - last_ok) <= 2000;
    bool connected = recent || recent_ok;

    auto header = ftxui::hbox({
        ftxui::text("kpacket monitor ") | ftxui::bold,
        ftxui::text("- "),
        ftxui::text(connected ? "connected" : "disconnected"),
        ftxui::text(" - "),
        ftxui::text(model.sub_endpoint)
    });

    // Body: scrolling text lines of incoming packets
    std::vector<ftxui::Element> body_lines;
    {
      std::lock_guard<std::mutex> lk(model.mtx);
      body_lines.reserve(model.rows.size());
      for (auto& r : model.rows) {
        char idbuf[8];
        std::snprintf(idbuf, sizeof(idbuf), "0x%04X", r.id);
        std::string line = r.time_str + " #" + std::to_string(r.local_seq);
        if (r.remote_msg_id) line += " mid=" + std::to_string(r.remote_msg_id);
        if (r.gap) line += " GAP";
        line += " " + r.dir + " " + std::string(idbuf) + " " + r.name + " size=" + std::to_string(r.size);
        body_lines.push_back(ftxui::text(std::move(line)));
      }
    }
    auto body = ftxui::vbox(std::move(body_lines)) | ftxui::frame; // no border

    // Footer (one line): totals and rate
    uint64_t total = model.total_packets.load(std::memory_order_relaxed);
    double rate = model.last_batch_rate.load(std::memory_order_relaxed);
    bool warmup = model.total_packets.load(std::memory_order_relaxed) < 10;
    if (warmup || !connected) rate = 0.0;
    char ratebuf[64];
    std::snprintf(ratebuf, sizeof(ratebuf), "pkts=%llu  rate=%.2f/s", static_cast<unsigned long long>(total), rate);
    auto rate_el = ftxui::text(ratebuf);
    if (warmup || !connected) rate_el = rate_el | ftxui::dim;
    auto footer = ftxui::hbox({ rate_el });

    return ftxui::vbox({ header, body | ftxui::flex, footer });
  });

  // Quit on 'q' or ESC
  main = ftxui::CatchEvent(main, [&](const ftxui::Event& e) {
    if (e == ftxui::Event::Character('q') || e == ftxui::Event::Escape) {
      model.running = false;
      screen.Exit();
      return true;
    }
    return false;
  });

  screen.Loop(main);
  model.running = false;
  return 0;
}


