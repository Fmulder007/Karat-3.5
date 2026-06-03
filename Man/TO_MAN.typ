#set document(title: "Карат-3,5
Техническое описание и инструкция по эксплуатации")
#set text(
  font: "New Computer Modern",
  lang: "Ru",
  region: "RU")
#set page(paper: "a5")
#show block: set align(center+horizon) 
#block (
#title()
#image("IMG_6905-1.jpg")
)


#set page(
  //numbering: "1",
  header: [
    #set text(8pt)
    Карат-3,5
  #h(1fr)
  Техническое описание и инструкция по эксплуатации],
  footer: context [#set text(8pt)
  #h(1fr)#counter(page).display("1")])
= Test
