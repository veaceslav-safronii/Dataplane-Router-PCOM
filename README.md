SAFRONII VEACESLAV 325-CD

# TEMA 1 - DATAPLANE ROUTER
Rezolvarea temei a fost bazata pe rezolvarea laboratorului 4


## ---Forwarding--

Este primit un pachet si sunt extrase header-ile necesare.

Se verifica Ether Type daca corespunde celui de IPV4 sau ARP,
in caz ca nu corespunde, pachetul este aruncat.

Daca e un pachet IP, este verificata integritatea acestuia, prin recalcularea
checksum-ului pentru antetul IP si este dat drop la pachetul corupt.

Se face cautarea in tabela de rutare pentru obtinerea adresei ip a
urmatorului hop, in caz ca nu se gaseste nimic pachetul este aruncat si
se transmite un mesaj ICMP.

Se verifica campul TTL, daca este mai mare de 1, acesta este decrementat si
se actualizeaza campul de checksum.

In caz contrar, se da drop la pachet si este transmis un mesaj ICMP
corespunzator.

Cu ajutorul protocolului ARP se obtine adresa MAC a urmatorului hop.

In cazul in care adresa MAC este deja in cache, sunt actualizate adresa
destinatie si sursa si pachetul este trimis catre interfata urmatorului hop.


## ---Longest Prefix Match-

Tabela de rutare este mai intai sortata crescator dupa prefixe,
apoi mai este sortata descrescator dupa masti.

Se aplica algoritmul de cautare binara pe tabela, fiind comparat prefixul cu
rezultatul operatiei AND pe ip-ul destinatie si masca curenta.

In cazul in care sunt egale, este returnata intrarea curenta din tabela.
Daca prefixul este mai mare, se cauta mai departe in partea stanga a tabelei,
altfel in partea dreapta.


## ---ICMP-

Avem o functie de icmp handler care construieste mesajul icmp,
in dependenta de tipul acestuia.

Pentru cazul in care TTL-ul este <= 1 este trimis un mesaj Time Exceeded.
Cand nu este gasita o ruta pana la destinatie este trimis
un mesaj Destination unreachable.

Pentru aceste doua cazuri se adauga pe langa headerele ether, ip, icmp si
header-ul ip vechi si inca 64 de biti dupa acesta.

In cazul primirii unui Echo Request este trimis un Echo Reply, pentru acesta
sunt copiati numerele de secventa si de identificare din request.

## ---ARP-

Pentru pachete de tip IP se verifica daca exista adresa MAC pentru urmatorul
hop, in caz ca nu exista pachetul este pus in coada si este trimis un arp_request.

Daca router-ul primeste un ARP request destinat acestuia, acesta trimite inapoi
un ARP reply, fiind adaugata adresa MAC a routerului.

Cand este primit un ARP reply este adaugata intrarea in cache (tabela) si sunt
scoase si trimise toate pachetele din coada ce au ca ip destinatie corespunzator celuia
din reply.


