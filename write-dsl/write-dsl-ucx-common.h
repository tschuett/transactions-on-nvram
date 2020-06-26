namespace {

template <typename Last>
void fillMessage(struct CommonMessage &Message, unsigned Idx, Last last) {
  Message.Flush[Idx].Window = last.getWindow();
  Message.Flush[Idx].Offset = last.getOffset();
  Message.Flush[Idx].Size = last.getSize();
}

template <class First, class... Rest>
void fillMessage(struct CommonMessage &Message, unsigned Idx, First first,
                 Rest... rest) {
  Message.Flush[Idx].Window = first.getWindow();
  Message.Flush[Idx].Offset = first.getOffset();
  Message.Flush[Idx].Size = first.getSize();
  fillMessage(Message, Idx + 1, rest...);
}

template <typename Last> void flushArgs(Last last) { last.flush(); }

template <class First, class... Rest>
void flushArgs(First first, Rest... rest) {
  first.flush();
  flushArgs(rest...);
}

}; // namespace

template <typename AType, typename VType> class WriteOp {
  AType Addr;
  VType v;

public:
  WriteOp(AType address, VType value) : Addr(address), v(value) {}

  void flush() { Addr.writeValue(v.readValue()); }

  uint8_t getSize() const { return v.getSize(); }

  uint32_t getOffset() { return Addr.getOffset(); }

  WindowKind getWindow() const { return Addr.getWindow(); }
};

template <class... Types> void flushOp(FlushKind Kind, Types... args) {
  struct CommonMessage Message;

  Message.tag = Tag::FLUSH;
  Message.Count = sizeof...(Types);
  static_assert(sizeof...(Types) <= CommonMessage::Slots);
  fillMessage(Message, 0, args...);
  flushArgs(args...);
  if (ucx::config::Flush)
    ucx::config::Comm->sendFlushMessage(Message);
}
