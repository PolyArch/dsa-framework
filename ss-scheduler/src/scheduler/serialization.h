#define SERIALIZABLE(T) \
  template void T::serialize(boost::archive::text_oarchive&, unsigned);\
  template void T::serialize(boost::archive::text_iarchive&, unsigned);
