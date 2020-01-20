

create or replace function clean_placenames(name text)
returns text
as $body$

begin

  return regexp_replace(name, '(, M\.D\.|, County|^Village|^(United )?Townships?|^The Corporation Of The Municipality|(, )?Municipality|^Rm|^Town|^District|^City)( of ?)| Additional|TNO aquatique de la MRC (de L''|de la|de|du|Le) | County||^Town Of |^DSL de |^CFB |\([^\)]+\)|, Unorganized', '', 'ig');

end;
$body$
language 'plpgsql';
