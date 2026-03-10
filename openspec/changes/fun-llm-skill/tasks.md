## 1. Skill Template Creation

- [ ] 1.1 Create skill content outline
- [ ] 1.2 Document all fun commands
- [ ] 1.3 Document all flags and options
- [ ] 1.4 Include usage examples
- [ ] 1.5 Include fun.ini format reference
- [ ] 1.6 Include common workflows
- [ ] 1.7 Include troubleshooting section

## 2. Template Embedding

- [ ] 2.1 Create templates/skills/fun-cli/SKILL.md.tmpl
- [ ] 2.2 Convert template to C string format
- [ ] 2.3 Add to fun binary build
- [ ] 2.4 Create access function for template

## 3. Skill Scaffolding Integration

- [ ] 3.1 Modify cmd_init to include skill scaffolding
- [ ] 3.2 Create .opencode/skills/fun-cli/ directory
- [ ] 3.3 Write SKILL.md from embedded template
- [ ] 3.4 Scaffold alongside fundamental-expert skill

## 4. Testing and Validation

- [ ] 4.1 Test fun init includes fun-cli skill
- [ ] 4.2 Verify skill content is complete
- [ ] 4.3 Verify both skills are scaffolded
- [ ] 4.4 Test LLM can reference skill correctly
