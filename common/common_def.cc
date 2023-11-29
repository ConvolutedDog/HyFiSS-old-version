#include "common_def.h"

int kernel_info_m_next_uid = 0;

#ifdef USE_BOOST
void simple_mpi_test(int argc, char **argv) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;
  if (world.rank() == 0) {
    /* Send data in rank 0. */
    int data = 123;
    for (int i = 1; i < world.size(); i++) {
      world.send(i, i, data);
      std::cout << "rank " << 0 << " send data: " << data << " to rank: " << i << std::endl;
    }
  } else {
    /* Recieve data in rank > 0. */
    int recv_data;
    world.recv(0, world.rank(), recv_data);
    std::cout << "rank " << world.rank() << " recv data: " << recv_data << " from rank: " << 0 << std::endl;
  }
}
#endif