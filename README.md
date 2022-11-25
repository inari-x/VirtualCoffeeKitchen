# VirtualCoffeeKitchen
Digital twin for a virtual meeting room to interact with co-workers

# VirtualCoffeeKitchen
Digital twin for a virtual meeting room to interact with co-workers

```mermaid
graph TB
  subgraph microcontroller
    A[/Button\] 
  end  
  A-->|Send user ID associated with button| B[Server, hosts open Zoom meeting]
  subgraph Server
    B -->|"POST https://api.zoom.us/chat/users/{userId}/messages"| C[Zoom Server]
  end
  
  C -->|Send invite link via chat bot| D[Laptop]
  D -->|Pop up chat message with join link| E
  E([User])-->|"Click link and click join"| D
```
